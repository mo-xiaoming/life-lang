#!/bin/sh
sed -i 's/-Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast//g' compile_commands.json
