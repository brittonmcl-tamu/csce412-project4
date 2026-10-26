"""Run the interactive program against isolated configuration and output files."""
from pathlib import Path
import re
import subprocess
import tempfile

binary = Path("LB").resolve()
base = """NUMBER_OF_WEBSERVERS=2
TOTAL_CYCLES=80
CYCLES_BETWEEN_UPDATES=2
REQUEST_FREQUENCY=100
BATCH_MIN=2
BATCH_MAX=2
TASK_MIN=1
TASK_MAX=4
BLOCKED_IPS=0
RANDOM_SEED=42
MODE=quiet
LOG_FILE=run.log
"""
with tempfile.TemporaryDirectory() as directory:
    folder = Path(directory)
    config = folder / "config.txt"
    def run(input="\n\n"):
        return subprocess.run([str(binary)], cwd=folder, input=input,
                              capture_output=True, text=True, timeout=10)
    config.write_text(base)
    first = run()
    assert first.returncode == 0, first.stderr
    assert "Elapsed cycles: 80" in first.stdout
    assert "Starting queue size: 200" in first.stdout
    assert "Work conservation: PASS" in first.stdout
    assert "[cycle" not in first.stdout
    assert first.stdout.endswith((folder / "run.log").read_text())
    assert run().stdout == first.stdout, "Fixed seed must reproduce output"

    config.write_text(base.replace("MODE=quiet", "MODE=verbose").replace("COLOR=0", "COLOR=1") + "COLOR=1\n")
    verbose = run()
    assert verbose.returncode == 0, verbose.stderr
    assert "ASSIGNED" in verbose.stdout and "COMPLETED" in verbose.stdout
    assert "QUEUED" in verbose.stdout and "SCALE UP" in verbose.stdout
    assert "\x1b[" in verbose.stdout and "\x1b[" not in (folder / "run.log").read_text()

    # No arrivals and one-cycle tasks must drain all accepted work and scale down.
    config.write_text(base.replace("REQUEST_FREQUENCY=100", "REQUEST_FREQUENCY=0").replace("TASK_MAX=4", "TASK_MAX=1").replace("MODE=quiet", "MODE=verbose"))
    drained = run("2\n1000\n")
    assert drained.returncode == 0, drained.stderr
    assert "Ending queue size: 0" in drained.stdout
    assert "Completed requests: 200\n" in drained.stdout
    assert "Ending servers: 1" in drained.stdout
    assert "SCALE DOWN" in drained.stdout

    # Scaling must occur on interval boundaries, using the current pool size.
    config.write_text(base.replace("CYCLES_BETWEEN_UPDATES=2", "CYCLES_BETWEEN_UPDATES=3").replace("MODE=quiet", "MODE=verbose"))
    cooldown = run()
    actions = re.findall(r"\[cycle (\d+)\] SCALE (?:UP|DOWN) servers=(\d+)", cooldown.stdout)
    assert actions and all(int(cycle) % 3 == 0 for cycle, count in actions)
    assert actions[0] == ("3", "3")

    for bad in ("REQUEST_FREQUENCY=101", "CYCLES_BETWEEN_UPDATES=0", "TASK_MIN=0",
                "NUMBER_OF_WEBSERVERS=-1", "UNKNOWN=3", "TOTAL_CYCLES=oops",
                "BLOCKED_IPS=2.0.0.0-1.0.0.0", "BLOCKED_IPS=0.0.0.0-255.255.255.255"):
        config.write_text(base + bad + "\n")
        result = run()
        assert result.returncode != 0 and "Error:" in result.stderr, bad
    config.write_text(base.replace("=", " "))
    assert run().returncode == 0
    assert run("0\n80\n").returncode != 0
    assert run("2\nnope\n").returncode != 0
    assert run("").returncode != 0
    for output in ("config.txt", "./config.txt", str(config.resolve()), "alias.txt", "hardlink.txt"):
        config.write_text(base.replace("LOG_FILE=run.log", "LOG_FILE=" + output))
        if output == "alias.txt": (folder / output).symlink_to(config)
        elif output == "hardlink.txt": (folder / output).hardlink_to(config)
        original = config.read_bytes()
        result = run()
        assert result.returncode != 0 and "must not overwrite" in result.stderr, output
        assert config.read_bytes() == original
    config.unlink()
    assert "Could not open config.txt" in run().stderr
print("Input/configuration checks passed")
