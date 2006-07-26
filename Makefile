all:
	make -C libavutil
	make -C libavformat
	make -C libavcodec
	rm -f pmpmod/PMPMOD.elf
	rm -f pmpmod/PARAM.SFO
	rm -f pmpmod/EBOOT.PBP
	make -C pmpmod
	make -C pmpmod kxploit
	pack-pbp pmpmod/PMPMOD%/EBOOT.PBP pmpmod/PARAM.SFO pmpmod/pack-pbp/ICON0.PNG NULL NULL pmpmod/pack-pbp/PIC1.PNG NULL NULL NULL

clean:
	make -C libavutil clean
	make -C libavformat clean
	make -C libavcodec clean
	make -C pmpmod clean
	rm -f -r pmpmod/PMPMOD
	rm -f -r pmpmod/PMPMOD%
