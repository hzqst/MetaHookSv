cd /d "%~dp0"

copy global_template.props global.props

git submodule update --init --recursive