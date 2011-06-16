" credit to Bart Trojanowski
" http://www.jukie.net/bart/blog/vim-and-linux-coding-style

if exists("b:did_vimrc_local")
	finish
endif
let b:did_vimrc_local = 1

setlocal noexpandtab
setlocal tabstop=8
setlocal shiftwidth=8
setlocal softtabstop=8
setlocal textwidth=78
setlocal formatoptions=tcqlron
setlocal cinoptions=:0,l1,t0,g0

