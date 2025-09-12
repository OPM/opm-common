cmake_minimum_required(VERSION 3.15)
# NOTE (2025-09-12): cmake_minimum_required must precede project() to avoid
# the developer warning: 'cmake_minimum_required() should be called prior to
# this top-level project() call.'
project(install_python_binding)

install(CODE "message(\"Dummy install\")")
