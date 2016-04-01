set makeprg="make"

map <leader>m <esc>:wa<cr>:Shell make<cr>
map <leader>b <esc>:wa<cr>:Shell make build<cr>
map <leader>r <esc>:wa<cr>:Shell make r<cr>
map <leader>d <esc>:wa<cr>:call system("xfce4-terminal -x sh -c 'gdb a.out -tui -ex \"b " . expand('%') . ":" . line(".") . " \" -ex \"r\" '")<cr>
