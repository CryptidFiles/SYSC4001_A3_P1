if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101299776_101287534_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101299776_101287534_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101299776_101287534_EP_RR.cpp