echo ========= Build docker image
docker build -t otus.lessons.28.02 .
echo ========= Execute ymr
docker run --rm -i otus.lessons.28.02 ymr test.in.txt 7 5
echo ========= Remove docker image
docker rmi otus.lessons.28.02