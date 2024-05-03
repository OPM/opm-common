import json
import sys

def generate_hpp_from_json(json_path: str, output_hpp_path: str, macro: str, namespace: str):
    with open(json_path, 'r', encoding='utf-8') as file:
        docstrings = json.load(file)

    hpp_content = f"""\
#ifndef {macro}
#define {macro}

// Generated docstrings
namespace {namespace} """
    hpp_content += "{"

    for func_name, info in docstrings.items():
        doc = info['doc'].replace('\n', '\n    ')
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

if __name__ == "__main__":
    # Check that exactly three command line arguments are provided
    if len(sys.argv) != 5:
        print("Usage: python generate_docstring_hpp.py <json_path> <output_hpp_path> <macro_name> <namespace>")
        sys.exit(1)
    # Extract json_path and output_hpp_path from command line arguments
    json_path = sys.argv[1]
    output_hpp_path = sys.argv[2]
    macro = sys.argv[3]
    namespace = sys.argv[4]
    generate_hpp_from_json(json_path, output_hpp_path, macro, namespace)
