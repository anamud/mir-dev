#!/bin/bash
find . -maxdepth 1 -name "profile-*.sh" -print | xargs sed -i 's/OPF/OFILE_PREFIX/g'
find . -maxdepth 1 -name "profile-*.sh" -print | xargs sed -i 's/CALLED_FUNCS/CALLED_FUNCTIONS/g'
find . -maxdepth 1 -name "profile-*.sh" -print | xargs sed -i 's/TASKS/OUTLINE_FUNCTIONS/g'
find . -maxdepth 1 -name "profile-*.sh" -print | xargs sed -i 's/PROCESS_TASK_GRAPH/PROCESS_PROFILE_DATA/g'
find . -maxdepth 1 -name "profile-*.sh" -print | xargs sed -i 's/scripts\/task-graph/programs\/common/g'

