number=0
commands='cat automate_equal'
while [ "$number" -lt $1 ]; do
    $commands|./server_coarse timeStats/equal_coarse
    number=$((number + 1))
done
number=0
while [ "$number" -lt $1 ]; do
    $commands|./server_rw timeStats/equal_rw
    number=$((number + 1))
done
number=0
while [ "$number" -lt $1 ]; do
    $commands|./server_fine timeStats/equal_fine
    number=$((number + 1))
done
