====pl实现1000M网口的样例：====

1. pl_net: gp0, hp0;
2. axi-net: [ps] ==>(gp0)==> [pl_net] ==> [net]
			[ddr] ==>(hp0)==> [pl_dma]
3. ref IP:
		"pg138-axi-ethernet(AXI 1G/2.5G Ethernet Subsystem LogiCORE IP Product Guide).pdf"
		"pg021-axi-dma(AXI DMA LogiCORE IP Product Guide).pdf"

