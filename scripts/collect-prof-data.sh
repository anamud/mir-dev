#!/bin/bash

find $MIR_ROOT/test -name "*graph*" -o -name "profile.s*" -o -name "*_stats" > prof-data-files.txt
tar -czvf prof-data.tgz --files-from=prof-data-files.txt
