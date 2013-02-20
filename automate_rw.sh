number=0
while [ "$number" -lt 10 ]; do
(echo "E test1 output/output1
E test2 output/output2
E WindowScript output/output3
w
"; cat)| ./server_rw

number=$((number + 1))
done


