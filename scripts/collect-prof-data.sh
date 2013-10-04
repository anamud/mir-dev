#!/bin/bash

find $MIR_ROOT -name '*graph*' -o -name 'profile.s*' -o -name '*_stats*' -print0 | tar -czvf prof-data.tar.gz -T - --null
