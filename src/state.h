enum {
	STATE_FIND_LAND = 1, //寻找空地中
	STATE_ENCLOSING = 2, //尽量大的圈地
	STATE_ESCAPE    = 3  //有威胁时逃跑(尽快脱离落笔态)
};
