# Always import kerasify (no extra dependencies)
from .kerasify import *

# Try to import scaler_layers (requires TensorFlow)
try:
    from .scaler_layers import *
except ImportError:
    import warnings
    warnings.warn(
        "Optional module 'scaler_layers' not loaded. "
        "Requires TensorFlow. Install 'tensorflow' to enable."
    )
