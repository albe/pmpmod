all:
	make -C libmpeg
	make -C libavutil
	make -C libavformat
	make -C libavcodec
	rm -f pmpmod/PMPMODAVC.elf
	rm -f pmpmod/PARAM.SFO
	rm -f pmpmod/EBOOT.PBP
	make -C pmpmod
	make -C pmpmod kxploit

clean:
	make -C libmpeg clean
	make -C libavutil clean
	make -C libavformat clean
	make -C libavcodec clean
	make -C pmpmod clean
	rm -f -r pmpmod/PMPMODAVC
	rm -f -r pmpmod/PMPMODAVC%
