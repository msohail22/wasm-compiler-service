# Code Execution Service (MVP)

Minimal C++ service that compiles and runs user-submitted C++ code against stored test cases.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/wasm_compiler_service
```

Server listens on `PORT` (default `8080`).

## Example Request

```bash
curl -X POST http://localhost:8080/execute \
  -H 'Content-Type: application/json' \
  -d '{"code":"#include <iostream>\nint main(){long long a,b; if(!(std::cin>>a>>b)) return 0; std::cout<<a+b<<"\\n";}","language":"cpp","question_id":"sum1"}'
```

## Response

JSON with `status`, `passed`, `total`, and per-case results.

## Notes

- Test cases are stored in `testcases.db` (SQLite). A sample `sum1` question is seeded on first run.
- The JSON parser is intentionally minimal and only supports string fields for the MVP.
- Each test case is executed in its own process with time and memory limits.
