__version__ = "1.0"

# Loads the C++ bindings into the namespace
from .cpp import *

# Promote Species to the main namespace
Species = species.Species

__all__ = (
    'boot',
    'restore',
    'Simulation',
    'Species'
)
