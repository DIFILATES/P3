#!/bin/bash
set -o pipefail
GETF0="get_pitch"

for pot in -55 -50 -45; do
    for r1 in 0.96 0.97 0.98; do
        for rmax in 0.38 0.40 0.42; do
            for zcr in 0.1 0.15 0.2 0.3; do
                for fwav in pitch_db/train/*.wav; do
                    ff0=${fwav/.wav/.f0}
                    $GETF0 -p $pot -1 $r1 -M $rmax -z $zcr "$fwav" "$ff0" > /dev/null
                done
                score=$(pitch_evaluate pitch_db/train/*.f0ref 2>/dev/null | grep "TOTAL" | awk '{print $3}')
                echo "$pot $r1 $rmax $zcr $score"
            done
        done
    done
done
