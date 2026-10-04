# ATmega128Demo::EepromPrmDb

The `ATmega128Demo::EepromPrmDb` component is a parameter database kept in the AVR's on-chip EEPROM. Components load
their parameters from it at boot through `getPrm`, and a component's `<PARAM>_PRM_SAVE` command stores the parameter
through `setPrm`.

## Introduction

* Saves are written in the background, one byte per `run` call, so a save never stalls a rate group.
* The database is checked at boot. If the header, any record, or its end marker is wrong, the whole database is marked
  as `CORRUPT`, and every component uses its defaults.
* `PRM_SAVE` always reports success, so the result of each save is shown in the `Status` telemetry channel.

## Requirements

| Name | Description | Rationale | Validation |
|---|---|---|---|
| EEPROMPRMDB-001 | The EepromPrmDb shall store parameter values in a configurable region of the on-chip EEPROM. | The rest of the EEPROM stays free for other data. | Hardware Test |
| EEPROMPRMDB-002 | The EepromPrmDb shall report every parameter as `INVALID` if, at boot, the database's header, any record, or its end marker is invalid. | All defaults is a known state; a mix of saved values and defaults isn't. | Hardware Test |
| EEPROMPRMDB-003 | The EepromPrmDb shall write saves in the background, without blocking the rate group. | An EEPROM byte takes about 8.5 ms to write, so writing a record all at once would stall the rate group. | Hardware Test |
| EEPROMPRMDB-004 | The EepromPrmDb shall read back every byte it writes, and report the state of the database and the result of each save in telemetry. | `PRM_SAVE` always reports success, so telemetry is the only way to know a save worked. | Hardware Test |

## Design

### Ports

| Kind | Name | Port Type | Description |
|---|---|---|---|
| sync input | getPrm | Fw.PrmGet | Gets a parameter's saved value. |
| sync input | setPrm | Fw.PrmSet | Queues a parameter's value to be saved. |
| sync input | run | Svc.Sched | Writes the next queued byte, one per call. |

### Commands

| Name | Description |
|---|---|
| `CLEAR_DB` | Writes a new header and an empty record list in the background, forgetting all saved parameters. Components keep their current values until the next boot. Use it to recover from `CORRUPT`. Responds `OK` once the format is queued, or `EXECUTION_ERROR` if another write is in progress; `Status` shows the result. |

### Telemetry

| Name | Type | Description |
|---|---|---|
| `Status` | `PrmDbStatus` | State of the database and the result of the last save. |

| `PrmDbStatus` | Meaning | Operator action |
|---|---|---|
| `EMPTY` | No saved parameters (blank EEPROM, or after `CLEAR_DB`). Components use defaults | None |
| `OK` | Saved parameters loaded, and the last save (if any) was successful | None |
| `WRITING` | A save or `CLEAR_DB` is being written. Saves sent now will be dropped | Wait for `OK` or `EMPTY` before sending another save |
| `SAVE_FAILED` | One or more saves weren't stored. Either they were dropped because they were sent while another write was in progress, or a byte of a new record read back wrong before the record was added to the database. Saved parameters are unchanged | Send every save since the last `OK` or `EMPTY` again, one at a time |
| `FULL` | The last save failed: no space left in the region. Saved parameters are unchanged | `CLEAR_DB`, then save the parameters again |
| `CORRUPT` | The database header, a record, or the end marker is invalid. Every parameter now loads as its default, so components that already loaded theirs keep them only until the next boot. Saves are refused | `CLEAR_DB`, then save the parameters again |

### EEPROM Layout

The region starts with a 3-byte header, `'P' 'D' 1` (the last byte is the layout version), followed by records back to
back, then an end marker byte, `0xFF` (the erased value). Each record:

| Field | Bytes | Contents |
|---|---|---|
| state | 1 | `0x5A` (valid) |
| id | 4 | Parameter ID, big endian |
| size | 1 | Value size, N |
| value | N | Serialized parameter value |
| crc | 2 | CRC-16/CCITT-FALSE of state, id, size and value, big endian |

State and size bytes are never rewritten once written, without clearing the database. If a parameter's saved size
changes (a string parameter's size depends on its value, and a firmware update can change a parameter's type), it is
appended as a new record. If multiple records with the same ID exist in the database, the last one is used.

### At Boot

`configure()` reads the whole database once, before components load their parameters. A blank region (a header of
`0xFF 0xFF 0xFF`, as on a new chip) is `EMPTY`; nothing is written until the first save. Otherwise the header must read
`'P' 'D' 1`, and every record must have a valid state byte, a size that fits, and a matching CRC, with an end marker
inside the region. If all of that holds, `Status` is `OK` (or `EMPTY` with no records). If anything fails, `Status` is
`CORRUPT`.

### Loading Parameters

When a component asks for a parameter through `getPrm`, `EepromPrmDb` returns the value from the parameter's last
record, or `INVALID` if there is none, in which case the component uses its default. While `Status` is `CORRUPT`, every
parameter is `INVALID`, so all components start from their defaults rather than from a mix of saved values and defaults.

### Saving a Parameter

A save arrives through `setPrm` when a component's `<PARAM>_PRM_SAVE` command runs. `EepromPrmDb` builds the new record
in RAM, decides where it goes, and queues its bytes; nothing is written yet. If the parameter's last record has the same
size, the value and CRC are rewritten in place. Otherwise the record is appended after the last one, followed by a new
end marker, with its state byte written last: until that byte is written, the record isn't part of the database, so a
save cut short by a reset leaves the old value in effect. On a blank region, a header is queued ahead of the record.

A save is refused while `Status` is `CORRUPT`, and dropped if another save or `CLEAR_DB` is still being written
(`Status` stays `WRITING`, then becomes `SAVE_FAILED` once that write finishes). If there's no space for a new record,
`Status` becomes `FULL` and nothing is queued.

### Writing in the Background

Writing an EEPROM byte takes about 8.5 ms on the ATmega128, so writing a whole record at once would stall the rate
group. Instead, each `run` call checks the byte it started on the previous call by reading it back, then starts the next
byte that differs from what's already in EEPROM, and returns without waiting. At 10 Hz, a new 4-byte parameter (13
bytes) takes about 1.3 s, and `Status` reads `WRITING` until it's done, then `OK`.

If a byte reads back wrong, the write stops with no retry. If the write was to the header, end marker, or an existing
record, the `Status` becomes `CORRUPT`. If the byte belongs to a new record, the database still has a valid end marker
prior to that record, so is not considered `CORRUPT`. In that case, `Status` becomes `SAVE_FAILED` and the save can be
reattempted.

`run` must be connected to a rate group. Otherwise the component never writes anything, reporting `WRITING` status
indefinitely after the first save.

## Configuration

Call `configure(regionStart, regionSize)` in `configureTopology()`, before `loadParameters()`, with the region of EEPROM
to use:

```cpp
prmDb.configure(0, E2END + 1);
```
