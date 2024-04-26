This folder contains the Python bindings and code required for embedding python for the OPM-common module of the Open Porous Media project.

For the Python bindings, see the [Python bindings of opm-simulators](https://github.com/OPM/opm-simulators/blob/master/python/README.md).

For embedding python, see the [documentation](https://opm-project.org/?page_id=1454).

To enable tooltips for opm_embedded on VSCode (embedded python code):
- Install opm-common
- Copy the file "<opm-common-folder>/python/opm_embedded/__init__.pyi" into the folder at "python.analysis.stubPath" of VS Code and rename it to "opm_embedded.pyi"
You can recreate this stub file by executing "stubgen -m opmcommon_python" in "<opm-common-build-folder>/python/opm", renaming the resuling stub file from "opmcommon_python.pyi" to "opm_embedded.pyi" and adding the lines
```python
	current_ecl_state: EclipseState
	current_report_step: int
	current_schedule: Schedule
	current_summary_state: SummaryState
```
