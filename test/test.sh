#!/usr/bin/env bash

i1=./ice10.png
i2=./will.png
i3=./wings_c.png

next() {
	printf "\033[32;1mcontinue ?\033[m"
	read
}

ask() {
	echo
	until [[ $# == 0 ]]; do
		printf "$1 "
		shift
	done
	next
}

query_n_exec() {
	echo
	printf "$1 (y/N) "
	read chc
	if [[ $chc == "y" ]]; then
		$2
	fi
}

## monitor list
test_mmonitor() {
	ask "echo monitor list"
	./we -ml
}

## wallpaper with --loop and --fifo
test_loop_n_fifo() {
	ask "set wallpaper"
	./we -m0 $i1 $i3 -t2

	ask "set wallpaper with --fifo"
	./we -m0 $i2 $i3 --fifo -t2

	ask "set wallpaper with --loop"
	./we -m0 $i1 $i2 --loop -t2

	ask "set wallpaper with --loop and --fifo"
	./we -m0 $i3 $i2 --loop --fifo -t2

	ask "set only one wallpaper with --loop"
	./we -m0 $i1 --loop -t2

	ask "set only one wallpaper with --loop and --fifo"
	./we -m0 $i2 --loop --fifo -t2
}

## wallpaper with --less-memory
test_less_memort() {
	ask "set only one wallpaper with --less-memory"
	./we -m0 $i1 --less-memory

	ask "set only one wallpaper with --less-memory and --fifo"
	./we -m0 $i3 --less-memory --fifo

	ask "set only one wallpaper with --less-memory and --loop"
	./we -m0 $i2 --less-memory --loop -t2

	ask "set wallpaper with --less-memory and --fifo"
	./we -m0 $i1 $i3 --less-memory --fifo -t2

	ask "set wallpaper with --less-memory and --loop"
	./we -m0 $i2 $i3 --less-memory --loop -t2

	ask "set wallpaper with --less-memory and --loop"
	./we -m0 $i1 $i2 $i3 --less-memory --loop -t2

	ask "set wallpaper with --less-memory and --loop and --fifo"
	./we -m0 $i1 $i2 --less-memory --loop -t2 --fifo
}

test_bgm() {
	ask "test bgm"
	./we -m0 $i3 $i1 -t 5 --loop --fifo --bgm ./b.wav
}

query_n_exec "test -ml ?" test_mmonitor
query_n_exec "test --loop and --fifo ?" test_loop_n_fifo
query_n_exec "test --less-memory ?" test_less_memort
query_n_exec "test --bgm ?" test_bgm

ask "all test done.."

