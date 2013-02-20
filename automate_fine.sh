number=0
while [ "$number" -lt 10 ]; do
(echo "E test1 output/output1
E test2 output/output2
E WindowScript output/output3
E test1 output/output4
E test2 output/output5
w
"; cat)| ./server_fine

number=$((number + 1))
done


