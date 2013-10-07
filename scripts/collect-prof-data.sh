#!/bin/bash

find $MIR_ROOT/test -type f -name "*graph*" -o -name "profile.s*" -o -name "profile-*" -o -name "*_stats" > prof-data-files.txt
tar -czvf prof-data.tgz --files-from=prof-data-files.txt
