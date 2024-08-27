# /bin/sh

make

mkfifo ag_fifo
mkdir ag_out

./httpd ag_fifo 9000 &

# download all .c files in local directory through httpd, compare to original
for file in *.c *.sh; do
    if [ -f "$file" ]; then
	timeout 2 curl -s -o "ag_out/$file" "http://localhost:9000/$file" || echo "error downloading $file" > "ag_out/$file"
	diff "ag_out/$file" "$file"
    fi
done

killall -QUIT httpd

killall -9 httpd

