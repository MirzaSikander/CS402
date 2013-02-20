number=0
while [ "$number" -lt 10 ]; do
    sh automate_equal_coarse.sh
done
number=0
while [ "$number" -lt 10 ]; do
    sh automate_equal_rw.sh
done
number=0
while [ "$number" -lt 10 ]; do
    sh automate_equal_fine.sh
done
