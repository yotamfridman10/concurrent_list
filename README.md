# Concurrent Linked List

A thread-safe sorted linked list in C, using fine-grained (hand-over-hand) locking so multiple operations can run concurrently. Includes an interactive command-driven test harness.

## Files

| File | Purpose |
|------|---------|
| `concurrent_list.c` / `concurrent_list.h` | The thread-safe sorted list implementation |
| `test.c` | Command-driven, multi-threaded test harness |
| `Makefile` | Build configuration |

## Build

```sh
make        # builds ./test
make clean  # removes the binary
```

Requires `gcc` with `pthread` support.

## Run

```sh
./test                  # interactive
./test < script.txt     # run a script of commands
```

Each command (except `create_list` and `join`) is dispatched on its own thread, so operations execute concurrently. Type `exit`, or end input (EOF), to stop.

## Commands

| Command | Action |
|---------|--------|
| `create_list` | Create the list |
| `insert_value <n>` | Insert `n` (kept sorted ascending) |
| `remove_value <n>` | Remove the first node equal to `n` |
| `print_list` | Print all values |
| `count_greater <n>` | Count values greater than `n` |
| `delete_list` | Destroy the list and free memory |
| `join` | Wait for all spawned threads to finish |
| `exit` | Quit |

### Example

```
create_list
insert_value 10
insert_value 4
insert_value 7
print_list        ->  4 7 10
remove_value 4
count_greater 5   ->  2 items were counted
join
exit
```

## Notes

- The list stays sorted; concurrent inserts/removes are made safe with per-node locks.
- `join` blocks until every thread spawned since the last `join` has completed — use it before reading results or deleting the list.
- The list is destroyed only by `delete_list`; pending operations are allowed to finish first.
