# opm-common: Python bindings and embedded Python

This folder contains the Python bindings for the OPM-common module and code required for using embedded Python with OPM.

- For the Python bindings, see also the [Python bindings of opm-simulators](https://github.com/OPM/opm-simulators/blob/master/Python/README.md).

- For the Python bindings and embedded Python code, see the [documentation](https://opm-project.org/?page_id=1454).

- To enable tooltips for embedded Python code (opm_embedded) for the editor VS Code, first make sure the stub file in `<opm-common-folder>/python/opm_embedded/__init__.pyi` is updated!
  You can recreate this stub file by executing
	```
	stubgen -m opmcommon_python -o . && cat opmcommon_python.pyi | sed "s/class Builtin:/current_ecl_state: EclipseState\ncurrent_report_step: int\ncurrent_schedule: Schedule\ncurrent_summary_state: SummaryState\n\nclass Builtin:/" > NAME-OF-OUTPUT-FILE.pyi
	```
	in `<opm-common-build-folder>/python/opm`. To execute stubgen, "mypy" is needed; this can be installed with "pip install mypy".

	The above stubgen command will create the stub file `opmcommon_python.pyi`. The cat command will copy the stub file `opmcommon_python.pyi` to "NAME-OF-OUTPUT-FILE.pyi" and add the lines
	```python
	current_ecl_state: EclipseState
	current_report_step: int
	current_schedule: Schedule
	current_summary_state: SummaryState
	```

  Then:
    - Either: Build the module opm-common and install it with "make install" - this will copy the file `<opm-common-folder>/python/opm_embedded/__init__.pyi` to the location where opm_embedded is installed, making it visible to VSCode.
    - Or: Copy the file `<opm-common-folder>/python/opm_embedded/__init__.pyi` into the folder defined in variable `python.analysis.stubPath` of VS Code and rename it to `opm_embedded.pyi`.

- So: **When updating any classes or documentation here, please also update the stub file located in `<opm-common-folder>/python/opm_embedded/__init__.pyi`!**
