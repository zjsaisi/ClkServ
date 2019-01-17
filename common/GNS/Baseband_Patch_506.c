
//****************************************************************************
// GloNav GPS Technology
// Copyright (C) 2008 GloNav Ltd.
// March House, London Rd, Daventry, Northants, UK.
// All rights reserved.
//
// Filename Baseband_Patch_506.c
//
// $Header: GNS/Baseband_Patch_506.c 1.1 2011/01/07 13:30:31PST Daniel Brown (dbrown) Exp  $
// $Locker:  $
//****************************************************************************


//****************************************************************************
//
// This file was created from base patch file E506P026.6CF8 on Mon Nov 02 15:29:09 2009
//
//****************************************************************************


#define PatchCheckSum             0x6CF8
#define PatchFileName     "E506P026.6CF8"
#define PatchRomVersion              506

#define N_1   64
static const U1 Comm1[N_1] = {   24,   24,   24,   24,   11,   11,   24,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   25,   25};
static const U2 Addr1[N_1] = { 7970, 7970, 7970, 7970,    1,    1, 4198,33414,33414,33415,33416,33417,33418,33419,33420,33421,33422,33423,33424,33425,
                              33426,33427,33428,33429,33430,33431,33432,33433,33434,33435,33436,33437,33438,33439,33440,33441,33442,33443,33444,33445,
                              33446,33447,33448,33449,33450,33451,33452,33453,33454,33455,33456,33457,33458,33459,33460,33461,33462,33463,33464,33465,
                              33466,40959,33439,33439};
static const U2 Data1[N_1] = {    0,    0,    0,    0,    0,    0,65535, 2844, 2844,39910,38400,38528,38656,54460, 8056,24090,33414,   89,24066, 8059,
                              36226,17793, 7010,26464,54460,19216,54460, 8057,16832, 8106,16832, 8121,17792,24089,65535,54716, 8056,24064, 4144,24088,
                              33415, 6984,24088,33466, 6984,24088,25625, 6984,26464, 6976,24089,65535,54716, 8056,16832,10813, 7936,26592,21793,17792,
                              17792,25625,    0,    0};

#define N_2   313
#define START_ADDR_2  33467
static const U2 Data2[N_2] = {22624, 6992,23361,   65,23361,26576, 1801, 1801, 6984,16832,10813, 7939,26592,17794, 8448,54460, 8056,24066, 8060, 1609,
                               8010, 1609, 8042,40609,20582, 1737, 1865, 7338,24470,40609,22436,40865,20566, 1609, 7170, 1545,40865,22452,23361,54712,
                               4145,38845,36280,17797,   91,24154,54656,24186,26576,22352,54712, 8057,22715,37381,21009,37125,20753,36869,20561,24089,
                              33411,36259,21521,17792,24089,33411,36259,20801,24089,33412,36259,16769,33467,17792,24089,33411,36259,20641,24089,33412,
                              36259,20577,24089,33413,36259,20514,16768,33467,54456, 8058,54460, 8056,17792,24066,33411,24065,33416,24067,33467,24068,
                              33779,54456, 8056,24089,65535,36025,17793,54716, 8056,24064, 4147,26464, 6992, 1794, 6288, 6248,  136,  136,24089,65535,
                              54716, 8056,22626,16832,10813, 7936,26592,16769,33517,17792,17792,24090, 2047,54460,16450,26464,54460,16450,17792,16832,
                              33598,17280,17792,54456,18948,38938,17794,24066, 4211, 7970,20530,54712, 7848, 7010,54716, 7838,36288,16368,20529,36288,
                              26000,20513, 8449,20496, 8454,54460, 7835,17792,54456,18948,38938,20529,16832,20580,20544,16832,33610,16832,20602,17792,
                              54456, 4209,52309,17794, 8618,54460, 4209,26464,54460, 4207,54460, 4211,16832,33610,54460, 7926,16832,22081,16832,33723,
                              33768,    4,24066,19025,33506,    1,16832,22210,33256,    4,16832,33762,17792,16768,22397,24128, 8971,16832,10432,24088,
                              29440,16832,33680,54523, 4210,24154,24088,29952,16832,33680,24162,36514,52232,20563,36032,65528,20517, 8449,20640,23298,
                              54460, 4207,32960, 5696,16832,33680,  144,22044, 8640,24160,17792, 8448,22464, 8457,16768,23092,24134,24070, 7881,24090,
                               8248,56460,24090, 8719,56461,24090, 9737,56463,24090,19547,56474,24090,17015,56469,24090, 6661,56459,24090, 3744,56453,
                              54456, 7835,52230,20626,24088, 2568,56486,24088, 7294,56479,24088, 7728,56480,24166,17792,24066,16439,32994, 9216,24066,
                               8031,32994,    1,12546,54716, 4115,54716, 4281,54456, 5845,54460, 5846,17792};

#define N_3   106
static const U1 Comm3[N_3] = {   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   25,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   25,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23};
static const U2 Addr3[N_3] = {33411,32769,32768,32836,38404,38405,38406,38407,38408,38409,38410,38411,38412,38413,38414,38415,38416,38417,38418,38419,
                              38420,38421,38422,38423,38402,38424,38425,38426,38427,38428,38429,38430,38431,38432,38433,38434,38435,38436,38437,38403,
                              38438,38439,38401,38400,33560,38404,38405,38406,38407,38408,38409,38410,38411,38412,38413,38414,38415,38416,38417,38418,
                              38419,38420,38421,38422,38423,38402,38424,38425,38426,38427,38428,38429,38430,38431,38432,38433,38434,38435,38436,38437,
                              38403,38438,38439,38401,38400,33560,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,
                              32769,32769,32769,32769,32769,32769};
static const U2 Data3[N_3] = {29523,31552,16768,33635,32786,23075,32770,33720,32788,22616,32772,22619,32802,    5,33357,33606,33356,16768,33381,33598,
                              33380,16768,33208,33682,38423, 4209,   85, 4207,    0,19200,23075,19202,22616,19216,    5, 4210,65530, 8057,    3,38437,
                              33598,33647,38439,21712,    0,32786,23075,32770,33720,32788,22616,32772,22619,32802,    5,33357,33606,33356,16768,33381,
                              33598,33380,16768,33208,33682,38423, 4209,   85, 4207,    0,19200,23075,19202,22616,19216,    5, 4210,65530, 8057,    3,
                              38437,33598,33647,38439,21712,    0,31552,31552,31552,31552,31552,31552,31552,31552,31552,31552,31552,31552,31552,31552,
                              31552,31552,31552,31552,31552,31552};

#define N_4   1980
#define START_ADDR_4  33780
static const U2 Data4[N_4] = {   79,   99,  116,   32,   50,   49,   32,   50,   48,   48,   57,   32,   49,   50,   58,   50,   57,   58,   50,   56,
                                  0,   65,   66,   67,   68,   76,   80,   32,   69,   53,   48,   54,   80,   48,   50,   54,   32,   82,  101,  108,
                                 32,    0,    7,    0,    1,    3,    7,   15,   31,24066,33413,24065,33418,24067,33780,24068,35759,16768,33568,16768,
                              33568,16768,33568,17792,16832,22081,16832,33723,16832,35045, 4880,17792,36032, 1855,20581,38690,20546,  149,36832,   10,
                              20672,52224,20578,38690,20609,  141,34784,  138,20544,38690,20514,33248,  128,17792,16832,22259, 8576,54521, 7905,54520,
                               4208,54460, 4208,54456, 8077,26592,20531,16832,22387, 8464,54460, 8077,54456, 7971,52228,17797, 8457,54460, 7971,17792,
                              24128,24129,24088,34194,54523, 7835,22586,24064, 7911,23555,33916, 1537, 8008,16832,33924,34785,    7,24088,22590,54460,
                               7970,24161,24160,17792,16768,22397,24128,24129,24131,24132,12032, 4944,21009,24088, 5152, 5952,24088, 4751, 5904,16832,
                              23074,24088, 4799, 5824, 8463,16832,23096, 4688,21089,24088, 5216, 5696,24088,    0,16832,10463,52225,21091, 4512,33472,
                                 63,22586,16832,34118,13057,20816,24067,  512, 4352,20881,12545,38682,30609,33472,   63,34489,22586,32960, 5696, 5248,
                               4160,20562,16832,34118,20640,16768,34001,16832,34037,52225,20563,16832,22943,16832,33762,26464,24164,24163,24161,24160,
                              17792,16832,10579,20609,23331,20578, 8449,16832,23096,16832,10579,20832,23331,20769,24154,24088,29440,16832,33924,54523,
                               4210,24185,36792,52488,20547,36288,65528,20501,20512,33250,  192,23298,35322,  192,17792,16768,22397,24088,  640, 6064,
                               8970,24067,  512,24088,29440, 5968,54523, 4210,26469,22586,13056,23297,34468,36868,20514,36516,36516,22586,26469,12607,
                              36025,26611,32960, 5696, 5664,24088, 9234, 5616,  140, 8703,16832,31238,24088, 9235, 5504, 8703,16832,31238,16832,34001,
                              20610, 4672,49856,30562,36320,   10,30674,52483,20675,36324,   20,21877,  144,21724,24064,  192,24066, 4206,32994,  128,
                              20592,54460, 4207, 8960,24066, 4206,33506,  128,24088, 9232, 4176,24088,  512, 4128,23360,17792,16768,22397,24128,24129,
                              24089,  259,16832,10587, 8961, 4480,24129,24064,65535, 4416,24186,56048,34465,36769,30613,22587,26368,32960, 5184,34467,
                               5744, 8703,16832,31238,16832,34001,23393,24161,24160,17792,23555,34173,23361,34464,22586,32960, 5184,34467, 5456, 8703,
                              16832,31238,16832,34001,24089,  256,34723,16832,10587,35322,  192,20497,54208,    0,17792,24128,24129,24131,24132,24088,
                               5888,16832,34116,38426,20594,24067,  512,33472,   63,22586,16832,34118,16768,33996, 2568, 2569, 2568, 2568, 2568, 2568,
                               2568, 7294, 7290, 7293, 7292, 7292, 7292, 7294, 7728, 7696, 7720, 7712, 7712, 7712, 7728,10884,10823,10966,10778,11002,
                              10952,10884,17344,24066,16432,32994, 8192,33514,   64,16832,23057,26464,54460,19025,54460, 8043,54460,16440,16768,  115,
                              54456, 5855,17793,24066,16434,33506, 4096,17792,24066,18946,32994,   48,17792,24064,16432,54712,18948,38939,20529,32992,
                               8192,20512,33504, 8192,24088,13331,16832,31277,24066,16432,33506,   64,24066,16435,32994,16384,16832,22304,16832,34248,
                              24162,17792,54456, 8081,16838,34287,17792,24088,  768,16832,22397,24154,32960,  640,16832,22397,24069, 8079,13057, 4352,
                              24069, 8080,13058, 4288,13056, 4944,24186,32960,  512,16832,22397,24088,11000,54460, 8081,17792, 4768, 7365,24089,  512,
                              36868,30561,22651,24088,17664,34467,16832,22397,49856,32960,17408,34467,22682, 9984,16832,34384,23322,20722,24134, 7365,
                              10048,16832,34384,24162,23322,20594,23298,34470,26368,22746, 6981, 4384,20560,24089,  768,16832,10587,26464,17792,24088,
                              15616,16832,22397,49916,32960,15360,32932,16832,22397,17792,23302,20515,26512,20496,50719,32932,24089,  770,34721,16832,
                              10587,16832,22397,17792,24088,16128,16832,22397,49855,32929,32960,15872,16832,22397, 5744, 8458,16832,31238,24088,28928,
                              16832,22397,22650,33953,12545,38426,30610,22587,23302,34465,52287,20915,36032,65505,20869,22746, 5392, 8458,16832,31238,
                              24088,28928,16832,22397,33955,38426,22226,38403,20514,23297,20565,23297,20531,26512,34470,22746,24088,    0,20496, 8449,
                              17792,54456, 8081,26592,54460, 8081,17792,28953,17792,28512,52228,20788,29053,44153,20737,34496, 5796,22618, 7938,39073,
                              26469, 8324, 3087, 3716,38588,52323,20518,24088,   99,24280,28952,17792,24187,30983,52488,26580,34466,22618,52481,20582,
                              52487,20518,52511,20518, 7970,20512, 7970,40952,40924,40932,28952,52740,16772, 2628,54456, 4195,47125,16771, 2628,59701,
                                128,30466,53968,52228,20518,24089,    1,16768, 2628,24160,24661,33088,29208,54527, 4168,17797,23307,20533,60696,   10,
                              20485, 1528,23359,60757,   10,20627,60696,   16,30676,60696,   32,30676,60696,   64,30676,60696,    4,30694,60696,    3,
                              30694,23547,56000,32273,46354,34744,59701,   32,20897,60696,   10,20853,54456, 5808,18368,36480,18816,29306,36480,39806,
                              36018,20516,36800,  512,29049,52744,20614,36800, 3072,26592,20550,36800,16384,26592,22448,15889,15378,31257,36288, 1000,
                              16772, 2913, 8448,14420,16768, 2953,16768, 2913,24089, 5208,31746, 2068,34731,22555,24088,   26,23561,34599, 7048, 6984,
                              17792,54712, 4149,22274,31237,52499,22226,24089, 7709,42754,22555,31254,36736,20531,12545,15433,22064,31236,52483,22018,
                              31361,52490,17797,55470,55727,16832, 7446,31133,14493,36537,14494,39765,39787,56048,44959,40956,42911,15519,29086,36537,
                              14494,55470,42565,55727,42822,39806,40958,39610,23307,39611,34475,16832, 5014,14407,55470,24645,14405,36512,55727,24646,
                              15430,36768,39806,40958,39610,23307,39611,34475,16832, 5014,14408,24088,    0,31134,40731,26577,40475,26577,49665,24088,
                                  0,31048,44871,20501,50690,14496,32332,46413,40897,37147,30673,29088,20497,50433,15948,15437,30286,46159,40929,26448,
                              14926,14415,29199,33472,    3,22778,24064, 1500,32832,28944,34475,52746,20516,34496, 6000,16832, 4941,16832, 5467,14467,
                              15490,60682,    2,16772,34841,30284,46157,16832, 5490,49727,16770,34841,30286,46159,16832, 5490,39914,49727,16770,34841,
                              33728,  255,52619,21298,60803,    1,21394,60802,   29,21346,24089, 5208,31746, 2068,34731,22555,30286,46159, 7048, 6976,
                              16832, 5524,24313,29264,15440,38845,36792,52481,20994,31049,20962,54992,26592,39746,14862,14351,28944,16832, 4941,52490,
                              20531,34752, 1199,20512,34752, 1179,15376, 8450,14346,60746,    3,21266, 8450,14410,21216,60803,    1,21170,60802,   29,
                              21122,58165,    4,21072,59701,    4,21026,28944,16832, 4941,52490,20531,34752, 1199,20512,34752, 1179,15376,30991,33728,
                              65532,15375, 8449,14467, 8477,14466,57653,    4,24089, 5208,31746, 2068,34731,22555,30286,46159, 7048, 6976,16832, 5524,
                              15440,60802,   29,17794,24089, 5208,31746, 2068,34731,42883,42883,22555,30284,46157, 7048, 6976,60803,    9,17794, 8448,
                              14409,14411,24089, 5208,34731,22587,24089, 5488,34731,22619,24067,  255,23561,34890, 8073,38025,16832, 5490,31051,40959,
                              35320,   63,20513,34752,  512,15435,39802,39920,33699, 7018, 6986, 8450,60682,    2,26581,14410, 8448,14409,60682,    2,
                              17794,29003,17794,28950,52254,17798,29240,36032,16128,17797, 8451,14346,17792,54712, 4151,14080,24088, 6086,42498,22618,
                              50702,22682,50702,22650,23355,21266,28949,54526, 4195,21206,12619,29062,20497,12569,28933,52243,21090, 8002,39044,46232,
                              26469,14488,36032, 1000,20627,30608, 8003,39044,46233,26469,14489,36032, 1000,20822,28939,34489,14347, 8449,16832, 9198,
                              10778,10903,10904,10905,54456, 4195,30997,35424,24064, 5888,14357,34528,    1,57653,  256, 8002,14484, 8003,14485, 8004,
                              14486, 6306, 6307, 6308,17792,24134,22733,34797,65506,    0,24128,24129,22534,34784,65506,22586,24096,   30,23296,16832,
                               6085,22592,20528,39752,32946, 6985, 7946,39114,23322,22419,23297,24161,24160,22950,    0,24166,17792,24089,33780,24088,
                              56870, 5552,22618,36544,56870,23322,52235,20627,24089, 8224, 7018,23298,36544,56870,23322,52235,22422,24089,33807,24088,
                              56863, 5232,16832,35045,17792,16832,33843,17792,24134,22733,34797,65534,    0,24129,22598,34786,65534,22586,54456, 7978,
                              38588, 6986, 7058,24066, 7969, 7234,26464,24384,35072,22914,    0,    0,26464,54460, 5894,24090, 1000,16832,31238,55551,
                              39760,16510,52227,20630,16832,33207,23322,20562, 8449,16832,33203,16832,23040,23297,16832,23323,24161,22950,    0,24166,
                              17792,24128,24129,22554,23360,52227,20566,17344,16832,23057,16832,32835,16832,35138,36320,    4,20578, 8480,54521, 4197,
                              23322,20497, 8965, 9984,24090, 1000,16832,31238,  137,23361,52233,22406,23296,16832,23515,24161,24160,17792,16832,24213,
                              12548,54777, 4204,23355,20930,23322,20562,12552,54777, 4204,23355,20817,24089,16399,54716, 4197,24089,  512,54777, 4204,
                              23355,20610,24089,  768,54777, 4204,23355,20610,23322,20577,24088,16463,20496, 8480,54460, 4197, 8456,54521, 4197,23322,
                              20545,24066, 8003,33506, 1024,17792,24134,22733,34797,65529,    0,24128,24129,24131,24132,22586,22593,  138,23334, 7938,
                              30688,16832,14193,23322,16769,35295,55551,52228,21042,34785,    2,56318,23334, 7961,52994,16832,14452,23322,16769,35295,
                              55550,54460, 7795,16832,20816,34785,    3,23334, 7937,52995,16832,14452,23322,21457,55549,38588,52230,20518, 8454,56573,
                              55549,54460, 8082,21296,36346,   30,21202,22662,22534,34788,65531,34784,65529,34785,    2,12032, 7945,23332,16832,14338,
                              23322,21009,55547, 6984,  139,23363,52225,22342,55801,40898,24096,   12,24088, 4204,16832,31221,22598,34786,65530, 1288,
                                482,24096,  768,24088, 4204,16832,31221,24384,35295,24076,35138,    0,23297,16832,16228,24164,24163,24161,24160,22950,
                                  0,24166,17792,24135,54712, 6174,20722,54456, 6180,54527, 6179,23322,36032,  599,20598, 8451,54460, 6177,54716, 6178,
                              16768,35423,54712, 6174,23387,52225,20931,12545,54716, 6174,54456, 6171,20593, 8467,54460, 6177,26464,54460, 6178,20560,
                               8457,54460, 6177,54716, 6178,54456, 6169,16770,35423, 8457,54460, 6177, 8449,16768,35421,36347,    2,20529,54456, 6170,
                              20850,54456, 6171,20513, 8467,20496, 8463,54460, 6177, 8455,54460, 6178,54456, 6169,21250, 8457,54460, 6177,24088,33822,
                              54523, 8082,   90,21072,54712, 4154,23387,52243,20646,54716, 6177,52262,20710,52302,20515, 8479,20656, 8463,20624,54456,
                               6171,20513, 8467,20496, 8463,54460, 6177, 8511,54460, 6178,54456, 6169,20642,24089,33822, 8457,54779, 8082,54460, 6177,
                                122,54460, 6178,54456, 6174,38588,52225,20822,54456, 6169,20769,54712, 7799,54456, 6178,38588,40635,20518,54716, 6178,
                              54712, 6176,54456, 6178,38588,40635,20518,54716, 6178,12545,54779, 6177,26608,38588,39807,34496,  700,38588,16832, 6049,
                              30704,54523, 6180,54527, 6179,24066, 6182,26592,54716, 6184,38588, 6986, 7058,16832, 6049,24282,54460, 6181,36338,    4,
                              20529,36338,    7,21010,54712, 6180,54779, 6179,26608,38588,39793,34746,22779,54712, 6184,38845,40959,54779, 6184,20159,
                              30688,36338,    7,20514,54779, 6184,23315,36539,34739,24066, 6181,54460, 6179,54716, 6180,34530,65535,24167,17792,24129,
                              24065, 4199, 7201, 8456,54460, 4199,16832,18709,23297,54460, 4199,24161,17792,24134,22733,34797,65526,    0,24135,24128,
                              24129,24131,24132,24133,26464,24068, 6858,24067, 6867,56575,56566,56220, 7963,52227,16774,35720, 7940,54460, 6185,12545,
                              18303,56830,55550,52236,16771,35720,22779, 2076,23307,24069, 6861,23418,34725,56826,24089, 6859,34746,56825,24089, 6858,
                              22554,34746,34784, 6867,56824,56567,56220, 7960,52227,16774,35701,55544,22586, 7937,54526, 6185,16770,35701,24088, 6859,
                              18038,55801,22587,22618, 7937,23333, 7394,18294,22566,34785,65530, 7201,56573,22619,28512, 7937,23346,56572,56827, 7778,
                              23538,54712, 4194,38720,40635,20517,24096,    1,55548,38588,40635,20501, 1281,24066, 6865,23330,23298,18294,18039,22587,
                              22618, 8001,40578,20518,12545,56827,12643,24274,20737,23327,20865, 8453,18045,23335,36282,20931,34791,    5,55807,55549,
                              36007,20851,55547,20817,20784,23327,20609,24066, 4195, 7234,38720,40610,20676,55807,20640,24274,20609,54456, 4195,24282,
                              55548,38588,40626,20500,55806,23323,52322,20691, 8459,54460, 7826,55551,54460, 7827,55550,54460, 7828,54716, 7825,16832,
                              21927,12572,18298,56826, 8476,12572,18041,18296,56569,56824, 8476,12545,18039,18302,56567,56830,55550,52236,16774,35586,
                               8476,12545,18038,18303,56566,56831,34788,   28,55551,52234,16774,35551,24165,24164,24163,24161,24160,24167,22950,    0,
                              24166,17792,16832,34175,24089,32770,24088,    0,   98,34466,30672,36347,35759,22438,54460, 8056,54460, 8058,17792,    0};

#define N_5   155
static const U1 Comm5[N_5] = {   23,   23,   23,   24,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   25,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
                                 23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,   25,   24,   11,   11};
static const U2 Addr5[N_5] = {33413,32769,32768, 8030,38660,38661,38662,38663,38664,38665,38666,38667,38668,38669,38670,38671,38672,38673,38674,38675,
                              38676,38677,38678,38679,38680,38681,38682,38683,38684,38685,38686,38687,38688,38689,38690,38691,38692,38693,38694,38695,
                              38696,38697,38698,38699,38700,38701,38702,38703,38704,38705,38706,38707,38708,38709,38710,38711,38712,38713,38714,38715,
                              38658,38716,38717,38718,38719,38720,38721,38722,38723,38724,38725,38659,38726,38727,38728,38657,38656,33829,38660,38661,
                              38662,38663,38664,38665,38666,38667,38668,38669,38670,38671,38672,38673,38674,38675,38676,38677,38678,38679,38680,38681,
                              38682,38683,38684,38685,38686,38687,38688,38689,38690,38691,38692,38693,38694,38695,38696,38697,38698,38699,38700,38701,
                              38702,38703,38704,38705,38706,38707,38708,38709,38710,38711,38712,38713,38714,38715,38658,38716,38717,38718,38719,38720,
                              38721,38722,38723,38724,38725,38659,38726,38727,38728,38657,38656,33829, 4198,    1,    1};
static const U2 Data5[N_5] = {38255,31552,16768,    0,33353,35017,33352,16768,32820,33844,33120,35048,33106,35101,33178,33874,33399,33852,33398,16768,
                              32828,35303,33208,33926,33160,34222,33184,34240,33383,34253,33382,16768,33355,35742,33354,16768,33329,34441,33328,16768,
                              33313,34282,33312,16768,33148,35186,32806,35519,32842,35533,32904,34601,32888,34913,32771,  343,32787,  254,32802,    7,
                              38715, 8030,    0,19201,  254,19216,    7, 8030,    0, 8057,    7,38725,33900,35017,35742,38728, 5468,    0,33353,35017,
                              33352,16768,32820,33844,33120,35048,33106,35101,33178,33874,33399,33852,33398,16768,32828,35303,33208,33926,33160,34222,
                              33184,34240,33383,34253,33382,16768,33355,35742,33354,16768,33329,34441,33328,16768,33313,34282,33312,16768,33148,35186,
                              32806,35519,32842,35533,32904,34601,32888,34913,32771,  343,32787,  254,32802,    7,38715, 8030,    0,19201,  254,19216,
                                  7, 8030,    0, 8057,    7,38725,33900,35017,35742,38728, 5468,    0,   10,    0,    0};