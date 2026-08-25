# Custom C Shell (OSN MP1)

## Project Structure
The codebase follows a modular structure separated into `src/` files and `include/` headers for maintainability.
- **Part A Models:** `lexer.c`, `parser.c`, `prompt.c`, `main.c`
- **Part B Models:** `hop.c`, `reveal.c`, `peek.c`, `locate.c` 
- **Part C Models:** `pipeline.c`, `execute.c`

To build the shell, simply run:
```bash
make clean
make
```
This produces the local executable `shell.out`. Run it using `./shell.out`.

---

## Features Implemented

### Part A: Shell Input & Parsing
- **Custom Prompt**: Dynamically formats your prompt as `<user@hostname:cwd>`. Smoothly replaces your starting terminal directory with `~` dynamically.
- **Lexical Analysis (Lexer)**: Incorporates a robust "Maximal Munch" finite-state parsing engine processing exact constraints for escapes (`\`), quoting bounds (`"`, `'`), and pipeline operations (`|`, `;`, `>`, `<`, `>>`, `&`) safely capturing memory.
- **Syntactic Grammar**: Executes a rigorous recursive-descent right-linear parser logic, failing dynamically constructed edge cases precisely with `cshell: invalid syntax`.

### Part B: Shell Intrinsics (Built-ins)
All built-ins securely replace typical environment syscalls executing inline cleanly.
- `hop`: A Zoxide-inspired directory navigator. Supports `~`, `.`, `..`, and `-` strictly. Further, implements algorithmic **Frecency** (Frequency/Recency) searching. If you navigate to missing targets, it predicts intended locations dynamically by traversing `~/.cshell_frecency` historical logs.
- `reveal`: Custom traversal logic fetching directory structs mimicking `ls`. Implements `-a` (hidden logic) and heavily nested depth-first-search recursion via `-t`, perfectly applying ASCII lexicographic `qsort` constraints across files recursively.
- `peek`: A custom structured file reader cloning `cat`. Implements `-n` for tracking non-empty line layouts securely. Extremely memory-efficient reverse-chunk loading `lseek` algorithm cleanly executing the `-r` backwards-printing limitation cleanly up entirely to EOF.
- `locate`: Recreates target environment `which` constraints mapping locally before traversing iterative sequences directly down standard `$PATH` environments validating `X_OK` execution bits contextually.

### Part C: Command Execution & Piping
- **Pipeline ASTing (`|`)**: Transforms flat mapped lexical chunks into chained `Pipeline` AST groupings wrapping standard executions. Spawns iterative sets of `fork()` children bridging dynamically mounted local `pipe()` connections linearly to form POSIX-compliant chain pipelines.
- **External Execution**: Safely integrates `execv` / `execvp` combinations securely handling standard relative paths, falling back linearly through CWD before enforcing strict explicit path traversal using `%` overrides seamlessly.
- **Advanced IO Multiplexing (`<`, `>`, `>>`)**: Follows extreme POSIX constraints routing output tracking properly across `O_TRUNC` and `O_APPEND`. Securely spawns **background-multiplexer** (invisible) instances to recursively join chained inputs (`command < f1 < f2`) sequentially without stalling or memory leaking pipeline limits, while cloning stdout outputs linearly out identically mirrored blocks into deeply stacked outputs (`command > f1 > f2`).
