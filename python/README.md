This folder contains the Python bindings and code required for embedding python for the OPM-common module of the Open Porous Media project.

For the Python bindings, see the [Python bindings of opm-simulators](https://github.com/OPM/opm-simulators/blob/master/python/README.md).

For embedded python code, see the [documentation](https://opm-project.org/?page_id=1454).

To enable tooltips for embedded python code (opm_embedded) for the editor VS Code:
- Either: Install opm-common.
- Or: Copy the file `<opm-common-folder>/python/opm_embedded/__init__.pyi` into the folder defined in variable `python.analysis.stubPath` of VS Code and rename it to `opm_embedded.pyi`.

	You can recreate this stub file by executing
	```
	stubgen -m opmcommon_python -o . && cat opmcommon_python.pyi | sed "s/class Builtin:/current_ecl_state: EclipseState\ncurrent_report_step: int\ncurrent_schedule: Schedule\ncurrent_summary_state:\n\nclass Builtin:/" > opm_embedded.pyi
	```
	in `<opm-common-build-folder>/python/opm`.

	This will copy the resulting stub file `opmcommon_python.pyi` to "opm_embedded.pyi" and add the lines
	```python
	current_ecl_state: EclipseState
	current_report_step: int
	current_schedule: Schedule
	current_summary_state: SummaryState
	```
 
