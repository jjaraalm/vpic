from skbuild import setup

def find_version():
    return "1.0"

def cmake_args():

    # FIXME: Hardcoded CXX/C flags for gcc

    flags = {
        'cmake_build_type': 'Release',
        'enable_python': 'ON',
        'enable_simd_detection': 'ON',
        'enable_integrated_tests': 'OFF',
        'cmake_c_flags': '-O3 -rdynamic -fno-strict-aliasing',
        'cmake_cxx_flags': '-O3 -rdynamic -fno-strict-aliasing'
    }

    return [f'-D{key.upper()}={val}' for key, val in flags.items()]

setup(
    name="vpic", # Replace with your own username
    packages=['vpic'],
    version=find_version(),
    author="??",
    author_email="??",
    cmake_args=cmake_args(),
    cmake_languages=('CXX',),
    cmake_minimum_required_version="3.0",
    description="Vectorized Particle-In-Cell code for plasma simulation",
    url="https://github.com/lanl/vpic",
    python_requires='>=3.6',
    install_requires=[
        'numpy',
        'mpi4py',
        'progressbar2'
    ],
    setup_requires=[
        'cmake'
    ]
)
