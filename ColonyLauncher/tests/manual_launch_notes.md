# Manual validation: custom app launches

Steps to confirm custom applications and Python scripts launch correctly from the Add Application workflow.

## POSIX (Linux/macOS)
1. Launch Colony Launcher and choose **Add Application**.
2. Browse to a native executable (e.g., `/bin/echo`) and add it.
3. Select the new entry and click **Launch**. Confirm the process starts (e.g., observe output or process list).
4. Repeat the add flow with a simple Python script (for example, a script that writes to `/tmp/colony-test.txt`).
5. Verify the script runs using the configured interpreter (`python3` by default) and produces the expected output.

## Windows
1. Open **Add Application** and add a native executable such as `notepad.exe`.
2. Launch the entry and confirm the application opens.
3. Add a Python script (for example, one that writes to `%TEMP%\colony-test.txt`).
4. Launch the script and confirm it runs using the configured interpreter (`py -3` by default, with `python.exe` as fallback) and produces the expected output.

## Interpreter override
- To use a non-default interpreter, set `pythonInterpreter` in `settings.json` (located via `Application::ResolveSettingsPath`) to the desired command or full path. Re-launch and repeat the Python script validation.
