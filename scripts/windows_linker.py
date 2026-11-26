"""Run IDF's linker generator without cmd.exe's 8191-character limit."""
import json
import os
import subprocess
from pathlib import Path

Import("env")

if os.name == "nt":
    node = env.File("$BUILD_DIR/sections.ld")
    executor = node.get_executor()
    original = executor.action_list[0]
    if not hasattr(original, "process"):
        raise RuntimeError("The ESP-IDF linker action changed; review windows_linker.py")
    helper = Path(env.subst("$PROJECT_DIR")) / "scripts" / "run_ldgen.py"

    def generate_linker_file(target, source, env):
        commands, _, _ = original.process(target, source, env)
        if len(commands) != 1:
            raise RuntimeError("Expected a single ESP-IDF linker generation command")
        arguments = [str(part) for part in commands[0]]
        arguments = [part[1:-1] if part.startswith('"') and part.endswith('"') else part
                     for part in arguments]
        argument_file = Path(str(target[0])).with_suffix(".args.json")
        argument_file.write_text(json.dumps(arguments), encoding="utf-8")
        process_env = dict(os.environ)
        for key, value in env["ENV"].items():
            process_env[key] = os.pathsep.join(map(str, value)) if isinstance(value, (list, tuple)) else str(value)
        return subprocess.run(
            [arguments[0], str(helper), str(argument_file.resolve())],
            env=process_env, check=False, shell=False,
        ).returncode

    executor.set_action_list(env.Action(
        generate_linker_file, "Generating project linker script $TARGET (Windows argument file)"))
    env.Depends(node, [str(helper), str(Path(env.subst("$PROJECT_DIR")) / "scripts" / "windows_linker.py")])
