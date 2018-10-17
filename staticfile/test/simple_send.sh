#/bin/sh

for ((i = 1; i <= 100; i++))
do
    curl -X GET http://localhost:5000/static/index.html -vvv
done
