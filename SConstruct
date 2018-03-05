# Author: Jared Stark;   Created: Mon Aug 16 11:28:20 PDT 2004
# Description: SConstruct for building a cbp submission.

# The framework can also be built using SCons (http://www.scons.org) instead of Make.
# This file is the SCons equivalent of a Makefile.

env = Environment(
    CCFLAGS = '-g -Wall',
    CXXFLAGS = '-g -Wall'
    )

sources = Split("""
    cbp_inst.cc
    main.cc
    op_state.cc
    predictor.cc
    tread.cc
""")

env.Program('predictor', sources)

