#!/bin/bash
set -o pipefail
GETF0="get_pitch"

for pot in -55 -50 -45 -40; do
    for r1 in 0.94 0.95 0.96 0.97; do
        for rmax in 0.36 0.38 0.40 0.42; do
            for zcr in 0.18 0.20 0.22; do
                for clip in 0.0 0.1 0.2 0.3; do
                for method in 1; do
                    for m in 0 3 5 10 15; do
                        for fwav in pitch_db/train/*.wav; do
                            ff0=${fwav/.wav/.f0}
                            $GETF0 -p $pot -1 $r1 -M $rmax -z $zcr -c $clip -t $method -m $m "$fwav" "$ff0" > /dev/null
                        done
                        score=$(pitch_evaluate pitch_db/train/*.f0ref 2>/dev/null | grep "TOTAL" | awk '{print $3}')
                        echo "$pot $r1 $rmax $zcr $clip $method $m $score"
                    done
                    done
                done
            done
        done
    done
done | tee resultats_opt.txt

sort -k7 -rn resultats_opt.txt | head -10