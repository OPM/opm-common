"""
OPM Docstring Header Generator

This script generates C++ header files with docstrings from JSON configuration files.
It supports two JSON formats with automatic detection and backward compatibility:

1. TEMPLATE FORMAT (New):
   - Uses "simulators", "constructors", "common_methods" sections
   - Supports {{name}} and {{class}} placeholder expansion
   - Requires simulator_type parameter for expansion
   - Example: docstrings_simulators.json

   Template "doc" field usage:
   - Simulator-level "doc": Main class documentation (→ PyClassName_docstring)
   - Constructor-level "doc": Constructor method documentation (→ PyClassName_constructor_docstring)
   - Method-level "doc": Individual method documentation (→ method_name_docstring)

2. FLAT FORMAT (Legacy):
   - Direct key-value pairs with "signature", "doc", "type" fields
   - No template expansion
   - Works with existing files unchanged
   - Example: docstrings_common.json

Format Detection:
- Template format: Detected by presence of both "simulators" AND "common_methods" keys
- Flat format: Used for all other JSON structures (maintains backward compatibility)

Usage:
  python generate_docstring_hpp.py <json_path> <output_hpp_path> <macro_name> <namespace> [simulator_type]

  - simulator_type parameter is required only for template format files
  - simulator_type parameter is ignored for flat format files
"""

import json
import sys

def expand_template(template_dict, simulator_config):
    """Recursively replace {{name}} and {{class}} placeholders"""
    if isinstance(template_dict, dict):
        result = {}
        for key, value in template_dict.items():
            result[key] = expand_template(value, simulator_config)
        return result
    elif isinstance(template_dict, str):
        return (template_dict
                .replace("{{name}}", simulator_config["name"])
                .replace("{{class}}", simulator_config["class"]))
    else:
        return template_dict

def expand_for_simulator(config, simulator_type):
    """Convert template config to flat docstring structure for specific simulator"""
    if simulator_type not in config["simulators"]:
        raise ValueError(f"Unknown simulator type: {simulator_type}")

    sim_config = config["simulators"][simulator_type]
    result = {}

    # Add class docstring
    class_name = sim_config["class"]
    result[class_name] = {
        "doc": sim_config.get("doc", ""),
        "signature": f"opm.simulators.{sim_config['name']}",
        "type": "class"
    }

    # Add constructor docstrings
    for constructor_key, constructor_template in config.get("constructors", {}).items():
        expanded = expand_template(constructor_template, sim_config)
        full_key = f"{class_name}_{constructor_key}"
        result[full_key] = {
            "signature": expanded.get("signature_template", ""),
            "doc": expanded.get("doc", "")
        }

    # Add method docstrings
    for method_name, method_template in config.get("common_methods", {}).items():
        expanded = expand_template(method_template, sim_config)
        result[method_name] = {
            "signature": expanded.get("signature_template", ""),
            "doc": expanded.get("doc", "")
        }

    return result

def generate_hpp_from_json(json_path: str, output_hpp_path: str, macro: str, namespace: str, simulator_type: str = None):
    with open(json_path, 'r', encoding='utf-8') as file:
        docstrings = json.load(file)

    # Check if this is the new template format
    if "simulators" in docstrings and "common_methods" in docstrings:
        if not simulator_type:
            raise ValueError("Simulator type required for template format JSON")
        docstrings = expand_for_simulator(docstrings, simulator_type)
    # else: use docstrings as-is (backward compatibility for flat format)

    # Generate header file (existing code)
    hpp_content = f"""\
#ifndef {macro}
#define {macro}

// Generated docstrings
namespace {namespace} """
    hpp_content += "{"

    for func_name, info in docstrings.items():
        doc = info.get('doc', '').replace('\n', '\n    ')
        hpp_content += f"""
static constexpr char {func_name}_docstring[] = R\"doc(
{doc}
)doc\";\n"""
    hpp_content += "}"
    hpp_content += f"""\
 // namespace {namespace}

#endif // {macro}
"""

    with open(output_hpp_path, 'w', encoding='utf-8') as file:
        file.write(hpp_content)

    print(f"Generated {output_hpp_path} from {json_path}" + (f" for simulator type {simulator_type}" if simulator_type else ""))

if __name__ == "__main__":
    # Updated to accept optional 5th parameter
    if len(sys.argv) not in [5, 6]:
        print("Usage: python generate_docstring_hpp.py <json_path> <output_hpp_path> <macro_name> <namespace> [simulator_type]")
        sys.exit(1)

    json_path = sys.argv[1]
    output_hpp_path = sys.argv[2]
    macro = sys.argv[3]
    namespace = sys.argv[4]
    simulator_type = sys.argv[5] if len(sys.argv) == 6 else None

    generate_hpp_from_json(json_path, output_hpp_path, macro, namespace, simulator_type)
