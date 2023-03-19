/*
 *
 *	bd.cpp
 * 
 *	Chess board
 * 
 */

#include "bd.h"


mt19937_64 rgen(7724322UL);

/*
 *
 *	genhabd
 * 
 *	The zobrist-hash generator.
 *
 */

GENHABD genhabd;

/* this is from the polyglot book format spec */
#define U64(u) u##ULL
const uint64_t GENHABD::ahabdRandom[781] = {
	   U64(0x9D39247E33776D41), U64(0x2AF7398005AAA5C7), U64(0x44DB015024623547), U64(0x9C15F73E62A76AE2),
	   U64(0x75834465489C0C89), U64(0x3290AC3A203001BF), U64(0x0FBBAD1F61042279), U64(0xE83A908FF2FB60CA),
	   U64(0x0D7E765D58755C10), U64(0x1A083822CEAFE02D), U64(0x9605D5F0E25EC3B0), U64(0xD021FF5CD13A2ED5),
	   U64(0x40BDF15D4A672E32), U64(0x011355146FD56395), U64(0x5DB4832046F3D9E5), U64(0x239F8B2D7FF719CC),
	   U64(0x05D1A1AE85B49AA1), U64(0x679F848F6E8FC971), U64(0x7449BBFF801FED0B), U64(0x7D11CDB1C3B7ADF0),
	   U64(0x82C7709E781EB7CC), U64(0xF3218F1C9510786C), U64(0x331478F3AF51BBE6), U64(0x4BB38DE5E7219443),
	   U64(0xAA649C6EBCFD50FC), U64(0x8DBD98A352AFD40B), U64(0x87D2074B81D79217), U64(0x19F3C751D3E92AE1),
	   U64(0xB4AB30F062B19ABF), U64(0x7B0500AC42047AC4), U64(0xC9452CA81A09D85D), U64(0x24AA6C514DA27500),
	   U64(0x4C9F34427501B447), U64(0x14A68FD73C910841), U64(0xA71B9B83461CBD93), U64(0x03488B95B0F1850F),
	   U64(0x637B2B34FF93C040), U64(0x09D1BC9A3DD90A94), U64(0x3575668334A1DD3B), U64(0x735E2B97A4C45A23),
	   U64(0x18727070F1BD400B), U64(0x1FCBACD259BF02E7), U64(0xD310A7C2CE9B6555), U64(0xBF983FE0FE5D8244),
	   U64(0x9F74D14F7454A824), U64(0x51EBDC4AB9BA3035), U64(0x5C82C505DB9AB0FA), U64(0xFCF7FE8A3430B241),
	   U64(0x3253A729B9BA3DDE), U64(0x8C74C368081B3075), U64(0xB9BC6C87167C33E7), U64(0x7EF48F2B83024E20),
	   U64(0x11D505D4C351BD7F), U64(0x6568FCA92C76A243), U64(0x4DE0B0F40F32A7B8), U64(0x96D693460CC37E5D),
	   U64(0x42E240CB63689F2F), U64(0x6D2BDCDAE2919661), U64(0x42880B0236E4D951), U64(0x5F0F4A5898171BB6),
	   U64(0x39F890F579F92F88), U64(0x93C5B5F47356388B), U64(0x63DC359D8D231B78), U64(0xEC16CA8AEA98AD76),
	   U64(0x5355F900C2A82DC7), U64(0x07FB9F855A997142), U64(0x5093417AA8A7ED5E), U64(0x7BCBC38DA25A7F3C),
	   U64(0x19FC8A768CF4B6D4), U64(0x637A7780DECFC0D9), U64(0x8249A47AEE0E41F7), U64(0x79AD695501E7D1E8),
	   U64(0x14ACBAF4777D5776), U64(0xF145B6BECCDEA195), U64(0xDABF2AC8201752FC), U64(0x24C3C94DF9C8D3F6),
	   U64(0xBB6E2924F03912EA), U64(0x0CE26C0B95C980D9), U64(0xA49CD132BFBF7CC4), U64(0xE99D662AF4243939),
	   U64(0x27E6AD7891165C3F), U64(0x8535F040B9744FF1), U64(0x54B3F4FA5F40D873), U64(0x72B12C32127FED2B),
	   U64(0xEE954D3C7B411F47), U64(0x9A85AC909A24EAA1), U64(0x70AC4CD9F04F21F5), U64(0xF9B89D3E99A075C2),
	   U64(0x87B3E2B2B5C907B1), U64(0xA366E5B8C54F48B8), U64(0xAE4A9346CC3F7CF2), U64(0x1920C04D47267BBD),
	   U64(0x87BF02C6B49E2AE9), U64(0x092237AC237F3859), U64(0xFF07F64EF8ED14D0), U64(0x8DE8DCA9F03CC54E),
	   U64(0x9C1633264DB49C89), U64(0xB3F22C3D0B0B38ED), U64(0x390E5FB44D01144B), U64(0x5BFEA5B4712768E9),
	   U64(0x1E1032911FA78984), U64(0x9A74ACB964E78CB3), U64(0x4F80F7A035DAFB04), U64(0x6304D09A0B3738C4),
	   U64(0x2171E64683023A08), U64(0x5B9B63EB9CEFF80C), U64(0x506AACF489889342), U64(0x1881AFC9A3A701D6),
	   U64(0x6503080440750644), U64(0xDFD395339CDBF4A7), U64(0xEF927DBCF00C20F2), U64(0x7B32F7D1E03680EC),
	   U64(0xB9FD7620E7316243), U64(0x05A7E8A57DB91B77), U64(0xB5889C6E15630A75), U64(0x4A750A09CE9573F7),
	   U64(0xCF464CEC899A2F8A), U64(0xF538639CE705B824), U64(0x3C79A0FF5580EF7F), U64(0xEDE6C87F8477609D),
	   U64(0x799E81F05BC93F31), U64(0x86536B8CF3428A8C), U64(0x97D7374C60087B73), U64(0xA246637CFF328532),
	   U64(0x043FCAE60CC0EBA0), U64(0x920E449535DD359E), U64(0x70EB093B15B290CC), U64(0x73A1921916591CBD),
	   U64(0x56436C9FE1A1AA8D), U64(0xEFAC4B70633B8F81), U64(0xBB215798D45DF7AF), U64(0x45F20042F24F1768),
	   U64(0x930F80F4E8EB7462), U64(0xFF6712FFCFD75EA1), U64(0xAE623FD67468AA70), U64(0xDD2C5BC84BC8D8FC),
	   U64(0x7EED120D54CF2DD9), U64(0x22FE545401165F1C), U64(0xC91800E98FB99929), U64(0x808BD68E6AC10365),
	   U64(0xDEC468145B7605F6), U64(0x1BEDE3A3AEF53302), U64(0x43539603D6C55602), U64(0xAA969B5C691CCB7A),
	   U64(0xA87832D392EFEE56), U64(0x65942C7B3C7E11AE), U64(0xDED2D633CAD004F6), U64(0x21F08570F420E565),
	   U64(0xB415938D7DA94E3C), U64(0x91B859E59ECB6350), U64(0x10CFF333E0ED804A), U64(0x28AED140BE0BB7DD),
	   U64(0xC5CC1D89724FA456), U64(0x5648F680F11A2741), U64(0x2D255069F0B7DAB3), U64(0x9BC5A38EF729ABD4),
	   U64(0xEF2F054308F6A2BC), U64(0xAF2042F5CC5C2858), U64(0x480412BAB7F5BE2A), U64(0xAEF3AF4A563DFE43),
	   U64(0x19AFE59AE451497F), U64(0x52593803DFF1E840), U64(0xF4F076E65F2CE6F0), U64(0x11379625747D5AF3),
	   U64(0xBCE5D2248682C115), U64(0x9DA4243DE836994F), U64(0x066F70B33FE09017), U64(0x4DC4DE189B671A1C),
	   U64(0x51039AB7712457C3), U64(0xC07A3F80C31FB4B4), U64(0xB46EE9C5E64A6E7C), U64(0xB3819A42ABE61C87),
	   U64(0x21A007933A522A20), U64(0x2DF16F761598AA4F), U64(0x763C4A1371B368FD), U64(0xF793C46702E086A0),
	   U64(0xD7288E012AEB8D31), U64(0xDE336A2A4BC1C44B), U64(0x0BF692B38D079F23), U64(0x2C604A7A177326B3),
	   U64(0x4850E73E03EB6064), U64(0xCFC447F1E53C8E1B), U64(0xB05CA3F564268D99), U64(0x9AE182C8BC9474E8),
	   U64(0xA4FC4BD4FC5558CA), U64(0xE755178D58FC4E76), U64(0x69B97DB1A4C03DFE), U64(0xF9B5B7C4ACC67C96),
	   U64(0xFC6A82D64B8655FB), U64(0x9C684CB6C4D24417), U64(0x8EC97D2917456ED0), U64(0x6703DF9D2924E97E),
	   U64(0xC547F57E42A7444E), U64(0x78E37644E7CAD29E), U64(0xFE9A44E9362F05FA), U64(0x08BD35CC38336615),
	   U64(0x9315E5EB3A129ACE), U64(0x94061B871E04DF75), U64(0xDF1D9F9D784BA010), U64(0x3BBA57B68871B59D),
	   U64(0xD2B7ADEEDED1F73F), U64(0xF7A255D83BC373F8), U64(0xD7F4F2448C0CEB81), U64(0xD95BE88CD210FFA7),
	   U64(0x336F52F8FF4728E7), U64(0xA74049DAC312AC71), U64(0xA2F61BB6E437FDB5), U64(0x4F2A5CB07F6A35B3),
	   U64(0x87D380BDA5BF7859), U64(0x16B9F7E06C453A21), U64(0x7BA2484C8A0FD54E), U64(0xF3A678CAD9A2E38C),
	   U64(0x39B0BF7DDE437BA2), U64(0xFCAF55C1BF8A4424), U64(0x18FCF680573FA594), U64(0x4C0563B89F495AC3),
	   U64(0x40E087931A00930D), U64(0x8CFFA9412EB642C1), U64(0x68CA39053261169F), U64(0x7A1EE967D27579E2),
	   U64(0x9D1D60E5076F5B6F), U64(0x3810E399B6F65BA2), U64(0x32095B6D4AB5F9B1), U64(0x35CAB62109DD038A),
	   U64(0xA90B24499FCFAFB1), U64(0x77A225A07CC2C6BD), U64(0x513E5E634C70E331), U64(0x4361C0CA3F692F12),
	   U64(0xD941ACA44B20A45B), U64(0x528F7C8602C5807B), U64(0x52AB92BEB9613989), U64(0x9D1DFA2EFC557F73),
	   U64(0x722FF175F572C348), U64(0x1D1260A51107FE97), U64(0x7A249A57EC0C9BA2), U64(0x04208FE9E8F7F2D6),
	   U64(0x5A110C6058B920A0), U64(0x0CD9A497658A5698), U64(0x56FD23C8F9715A4C), U64(0x284C847B9D887AAE),
	   U64(0x04FEABFBBDB619CB), U64(0x742E1E651C60BA83), U64(0x9A9632E65904AD3C), U64(0x881B82A13B51B9E2),
	   U64(0x506E6744CD974924), U64(0xB0183DB56FFC6A79), U64(0x0ED9B915C66ED37E), U64(0x5E11E86D5873D484),
	   U64(0xF678647E3519AC6E), U64(0x1B85D488D0F20CC5), U64(0xDAB9FE6525D89021), U64(0x0D151D86ADB73615),
	   U64(0xA865A54EDCC0F019), U64(0x93C42566AEF98FFB), U64(0x99E7AFEABE000731), U64(0x48CBFF086DDF285A),
	   U64(0x7F9B6AF1EBF78BAF), U64(0x58627E1A149BBA21), U64(0x2CD16E2ABD791E33), U64(0xD363EFF5F0977996),
	   U64(0x0CE2A38C344A6EED), U64(0x1A804AADB9CFA741), U64(0x907F30421D78C5DE), U64(0x501F65EDB3034D07),
	   U64(0x37624AE5A48FA6E9), U64(0x957BAF61700CFF4E), U64(0x3A6C27934E31188A), U64(0xD49503536ABCA345),
	   U64(0x088E049589C432E0), U64(0xF943AEE7FEBF21B8), U64(0x6C3B8E3E336139D3), U64(0x364F6FFA464EE52E),
	   U64(0xD60F6DCEDC314222), U64(0x56963B0DCA418FC0), U64(0x16F50EDF91E513AF), U64(0xEF1955914B609F93),
	   U64(0x565601C0364E3228), U64(0xECB53939887E8175), U64(0xBAC7A9A18531294B), U64(0xB344C470397BBA52),
	   U64(0x65D34954DAF3CEBD), U64(0xB4B81B3FA97511E2), U64(0xB422061193D6F6A7), U64(0x071582401C38434D),
	   U64(0x7A13F18BBEDC4FF5), U64(0xBC4097B116C524D2), U64(0x59B97885E2F2EA28), U64(0x99170A5DC3115544),
	   U64(0x6F423357E7C6A9F9), U64(0x325928EE6E6F8794), U64(0xD0E4366228B03343), U64(0x565C31F7DE89EA27),
	   U64(0x30F5611484119414), U64(0xD873DB391292ED4F), U64(0x7BD94E1D8E17DEBC), U64(0xC7D9F16864A76E94),
	   U64(0x947AE053EE56E63C), U64(0xC8C93882F9475F5F), U64(0x3A9BF55BA91F81CA), U64(0xD9A11FBB3D9808E4),
	   U64(0x0FD22063EDC29FCA), U64(0xB3F256D8ACA0B0B9), U64(0xB03031A8B4516E84), U64(0x35DD37D5871448AF),
	   U64(0xE9F6082B05542E4E), U64(0xEBFAFA33D7254B59), U64(0x9255ABB50D532280), U64(0xB9AB4CE57F2D34F3),
	   U64(0x693501D628297551), U64(0xC62C58F97DD949BF), U64(0xCD454F8F19C5126A), U64(0xBBE83F4ECC2BDECB),
	   U64(0xDC842B7E2819E230), U64(0xBA89142E007503B8), U64(0xA3BC941D0A5061CB), U64(0xE9F6760E32CD8021),
	   U64(0x09C7E552BC76492F), U64(0x852F54934DA55CC9), U64(0x8107FCCF064FCF56), U64(0x098954D51FFF6580),
	   U64(0x23B70EDB1955C4BF), U64(0xC330DE426430F69D), U64(0x4715ED43E8A45C0A), U64(0xA8D7E4DAB780A08D),
	   U64(0x0572B974F03CE0BB), U64(0xB57D2E985E1419C7), U64(0xE8D9ECBE2CF3D73F), U64(0x2FE4B17170E59750),
	   U64(0x11317BA87905E790), U64(0x7FBF21EC8A1F45EC), U64(0x1725CABFCB045B00), U64(0x964E915CD5E2B207),
	   U64(0x3E2B8BCBF016D66D), U64(0xBE7444E39328A0AC), U64(0xF85B2B4FBCDE44B7), U64(0x49353FEA39BA63B1),
	   U64(0x1DD01AAFCD53486A), U64(0x1FCA8A92FD719F85), U64(0xFC7C95D827357AFA), U64(0x18A6A990C8B35EBD),
	   U64(0xCCCB7005C6B9C28D), U64(0x3BDBB92C43B17F26), U64(0xAA70B5B4F89695A2), U64(0xE94C39A54A98307F),
	   U64(0xB7A0B174CFF6F36E), U64(0xD4DBA84729AF48AD), U64(0x2E18BC1AD9704A68), U64(0x2DE0966DAF2F8B1C),
	   U64(0xB9C11D5B1E43A07E), U64(0x64972D68DEE33360), U64(0x94628D38D0C20584), U64(0xDBC0D2B6AB90A559),
	   U64(0xD2733C4335C6A72F), U64(0x7E75D99D94A70F4D), U64(0x6CED1983376FA72B), U64(0x97FCAACBF030BC24),
	   U64(0x7B77497B32503B12), U64(0x8547EDDFB81CCB94), U64(0x79999CDFF70902CB), U64(0xCFFE1939438E9B24),
	   U64(0x829626E3892D95D7), U64(0x92FAE24291F2B3F1), U64(0x63E22C147B9C3403), U64(0xC678B6D860284A1C),
	   U64(0x5873888850659AE7), U64(0x0981DCD296A8736D), U64(0x9F65789A6509A440), U64(0x9FF38FED72E9052F),
	   U64(0xE479EE5B9930578C), U64(0xE7F28ECD2D49EECD), U64(0x56C074A581EA17FE), U64(0x5544F7D774B14AEF),
	   U64(0x7B3F0195FC6F290F), U64(0x12153635B2C0CF57), U64(0x7F5126DBBA5E0CA7), U64(0x7A76956C3EAFB413),
	   U64(0x3D5774A11D31AB39), U64(0x8A1B083821F40CB4), U64(0x7B4A38E32537DF62), U64(0x950113646D1D6E03),
	   U64(0x4DA8979A0041E8A9), U64(0x3BC36E078F7515D7), U64(0x5D0A12F27AD310D1), U64(0x7F9D1A2E1EBE1327),
	   U64(0xDA3A361B1C5157B1), U64(0xDCDD7D20903D0C25), U64(0x36833336D068F707), U64(0xCE68341F79893389),
	   U64(0xAB9090168DD05F34), U64(0x43954B3252DC25E5), U64(0xB438C2B67F98E5E9), U64(0x10DCD78E3851A492),
	   U64(0xDBC27AB5447822BF), U64(0x9B3CDB65F82CA382), U64(0xB67B7896167B4C84), U64(0xBFCED1B0048EAC50),
	   U64(0xA9119B60369FFEBD), U64(0x1FFF7AC80904BF45), U64(0xAC12FB171817EEE7), U64(0xAF08DA9177DDA93D),
	   U64(0x1B0CAB936E65C744), U64(0xB559EB1D04E5E932), U64(0xC37B45B3F8D6F2BA), U64(0xC3A9DC228CAAC9E9),
	   U64(0xF3B8B6675A6507FF), U64(0x9FC477DE4ED681DA), U64(0x67378D8ECCEF96CB), U64(0x6DD856D94D259236),
	   U64(0xA319CE15B0B4DB31), U64(0x073973751F12DD5E), U64(0x8A8E849EB32781A5), U64(0xE1925C71285279F5),
	   U64(0x74C04BF1790C0EFE), U64(0x4DDA48153C94938A), U64(0x9D266D6A1CC0542C), U64(0x7440FB816508C4FE),
	   U64(0x13328503DF48229F), U64(0xD6BF7BAEE43CAC40), U64(0x4838D65F6EF6748F), U64(0x1E152328F3318DEA),
	   U64(0x8F8419A348F296BF), U64(0x72C8834A5957B511), U64(0xD7A023A73260B45C), U64(0x94EBC8ABCFB56DAE),
	   U64(0x9FC10D0F989993E0), U64(0xDE68A2355B93CAE6), U64(0xA44CFE79AE538BBE), U64(0x9D1D84FCCE371425),
	   U64(0x51D2B1AB2DDFB636), U64(0x2FD7E4B9E72CD38C), U64(0x65CA5B96B7552210), U64(0xDD69A0D8AB3B546D),
	   U64(0x604D51B25FBF70E2), U64(0x73AA8A564FB7AC9E), U64(0x1A8C1E992B941148), U64(0xAAC40A2703D9BEA0),
	   U64(0x764DBEAE7FA4F3A6), U64(0x1E99B96E70A9BE8B), U64(0x2C5E9DEB57EF4743), U64(0x3A938FEE32D29981),
	   U64(0x26E6DB8FFDF5ADFE), U64(0x469356C504EC9F9D), U64(0xC8763C5B08D1908C), U64(0x3F6C6AF859D80055),
	   U64(0x7F7CC39420A3A545), U64(0x9BFB227EBDF4C5CE), U64(0x89039D79D6FC5C5C), U64(0x8FE88B57305E2AB6),
	   U64(0xA09E8C8C35AB96DE), U64(0xFA7E393983325753), U64(0xD6B6D0ECC617C699), U64(0xDFEA21EA9E7557E3),
	   U64(0xB67C1FA481680AF8), U64(0xCA1E3785A9E724E5), U64(0x1CFC8BED0D681639), U64(0xD18D8549D140CAEA),
	   U64(0x4ED0FE7E9DC91335), U64(0xE4DBF0634473F5D2), U64(0x1761F93A44D5AEFE), U64(0x53898E4C3910DA55),
	   U64(0x734DE8181F6EC39A), U64(0x2680B122BAA28D97), U64(0x298AF231C85BAFAB), U64(0x7983EED3740847D5),
	   U64(0x66C1A2A1A60CD889), U64(0x9E17E49642A3E4C1), U64(0xEDB454E7BADC0805), U64(0x50B704CAB602C329),
	   U64(0x4CC317FB9CDDD023), U64(0x66B4835D9EAFEA22), U64(0x219B97E26FFC81BD), U64(0x261E4E4C0A333A9D),
	   U64(0x1FE2CCA76517DB90), U64(0xD7504DFA8816EDBB), U64(0xB9571FA04DC089C8), U64(0x1DDC0325259B27DE),
	   U64(0xCF3F4688801EB9AA), U64(0xF4F5D05C10CAB243), U64(0x38B6525C21A42B0E), U64(0x36F60E2BA4FA6800),
	   U64(0xEB3593803173E0CE), U64(0x9C4CD6257C5A3603), U64(0xAF0C317D32ADAA8A), U64(0x258E5A80C7204C4B),
	   U64(0x8B889D624D44885D), U64(0xF4D14597E660F855), U64(0xD4347F66EC8941C3), U64(0xE699ED85B0DFB40D),
	   U64(0x2472F6207C2D0484), U64(0xC2A1E7B5B459AEB5), U64(0xAB4F6451CC1D45EC), U64(0x63767572AE3D6174),
	   U64(0xA59E0BD101731A28), U64(0x116D0016CB948F09), U64(0x2CF9C8CA052F6E9F), U64(0x0B090A7560A968E3),
	   U64(0xABEEDDB2DDE06FF1), U64(0x58EFC10B06A2068D), U64(0xC6E57A78FBD986E0), U64(0x2EAB8CA63CE802D7),
	   U64(0x14A195640116F336), U64(0x7C0828DD624EC390), U64(0xD74BBE77E6116AC7), U64(0x804456AF10F5FB53),
	   U64(0xEBE9EA2ADF4321C7), U64(0x03219A39EE587A30), U64(0x49787FEF17AF9924), U64(0xA1E9300CD8520548),
	   U64(0x5B45E522E4B1B4EF), U64(0xB49C3B3995091A36), U64(0xD4490AD526F14431), U64(0x12A8F216AF9418C2),
	   U64(0x001F837CC7350524), U64(0x1877B51E57A764D5), U64(0xA2853B80F17F58EE), U64(0x993E1DE72D36D310),
	   U64(0xB3598080CE64A656), U64(0x252F59CF0D9F04BB), U64(0xD23C8E176D113600), U64(0x1BDA0492E7E4586E),
	   U64(0x21E0BD5026C619BF), U64(0x3B097ADAF088F94E), U64(0x8D14DEDB30BE846E), U64(0xF95CFFA23AF5F6F4),
	   U64(0x3871700761B3F743), U64(0xCA672B91E9E4FA16), U64(0x64C8E531BFF53B55), U64(0x241260ED4AD1E87D),
	   U64(0x106C09B972D2E822), U64(0x7FBA195410E5CA30), U64(0x7884D9BC6CB569D8), U64(0x0647DFEDCD894A29),
	   U64(0x63573FF03E224774), U64(0x4FC8E9560F91B123), U64(0x1DB956E450275779), U64(0xB8D91274B9E9D4FB),
	   U64(0xA2EBEE47E2FBFCE1), U64(0xD9F1F30CCD97FB09), U64(0xEFED53D75FD64E6B), U64(0x2E6D02C36017F67F),
	   U64(0xA9AA4D20DB084E9B), U64(0xB64BE8D8B25396C1), U64(0x70CB6AF7C2D5BCF0), U64(0x98F076A4F7A2322E),
	   U64(0xBF84470805E69B5F), U64(0x94C3251F06F90CF3), U64(0x3E003E616A6591E9), U64(0xB925A6CD0421AFF3),
	   U64(0x61BDD1307C66E300), U64(0xBF8D5108E27E0D48), U64(0x240AB57A8B888B20), U64(0xFC87614BAF287E07),
	   U64(0xEF02CDD06FFDB432), U64(0xA1082C0466DF6C0A), U64(0x8215E577001332C8), U64(0xD39BB9C3A48DB6CF),
	   U64(0x2738259634305C14), U64(0x61CF4F94C97DF93D), U64(0x1B6BACA2AE4E125B), U64(0x758F450C88572E0B),
	   U64(0x959F587D507A8359), U64(0xB063E962E045F54D), U64(0x60E8ED72C0DFF5D1), U64(0x7B64978555326F9F),
	   U64(0xFD080D236DA814BA), U64(0x8C90FD9B083F4558), U64(0x106F72FE81E2C590), U64(0x7976033A39F7D952),
	   U64(0xA4EC0132764CA04B), U64(0x733EA705FAE4FA77), U64(0xB4D8F77BC3E56167), U64(0x9E21F4F903B33FD9),
	   U64(0x9D765E419FB69F6D), U64(0xD30C088BA61EA5EF), U64(0x5D94337FBFAF7F5B), U64(0x1A4E4822EB4D7A59),
	   U64(0x6FFE73E81B637FB3), U64(0xDDF957BC36D8B9CA), U64(0x64D0E29EEA8838B3), U64(0x08DD9BDFD96B9F63),
	   U64(0x087E79E5A57D1D13), U64(0xE328E230E3E2B3FB), U64(0x1C2559E30F0946BE), U64(0x720BF5F26F4D2EAA),
	   U64(0xB0774D261CC609DB), U64(0x443F64EC5A371195), U64(0x4112CF68649A260E), U64(0xD813F2FAB7F5C5CA),
	   U64(0x660D3257380841EE), U64(0x59AC2C7873F910A3), U64(0xE846963877671A17), U64(0x93B633ABFA3469F8),
	   U64(0xC0C0F5A60EF4CDCF), U64(0xCAF21ECD4377B28C), U64(0x57277707199B8175), U64(0x506C11B9D90E8B1D),
	   U64(0xD83CC2687A19255F), U64(0x4A29C6465A314CD1), U64(0xED2DF21216235097), U64(0xB5635C95FF7296E2),
	   U64(0x22AF003AB672E811), U64(0x52E762596BF68235), U64(0x9AEBA33AC6ECC6B0), U64(0x944F6DE09134DFB6),
	   U64(0x6C47BEC883A7DE39), U64(0x6AD047C430A12104), U64(0xA5B1CFDBA0AB4067), U64(0x7C45D833AFF07862),
	   U64(0x5092EF950A16DA0B), U64(0x9338E69C052B8E7B), U64(0x455A4B4CFE30E3F5), U64(0x6B02E63195AD0CF8),
	   U64(0x6B17B224BAD6BF27), U64(0xD1E0CCD25BB9C169), U64(0xDE0C89A556B9AE70), U64(0x50065E535A213CF6),
	   U64(0x9C1169FA2777B874), U64(0x78EDEFD694AF1EED), U64(0x6DC93D9526A50E68), U64(0xEE97F453F06791ED),
	   U64(0x32AB0EDB696703D3), U64(0x3A6853C7E70757A7), U64(0x31865CED6120F37D), U64(0x67FEF95D92607890),
	   U64(0x1F2B1D1F15F6DC9C), U64(0xB69E38A8965C6B65), U64(0xAA9119FF184CCCF4), U64(0xF43C732873F24C13),
	   U64(0xFB4A3D794A9A80D2), U64(0x3550C2321FD6109C), U64(0x371F77E76BB8417E), U64(0x6BFA9AAE5EC05779),
	   U64(0xCD04F3FF001A4778), U64(0xE3273522064480CA), U64(0x9F91508BFFCFC14A), U64(0x049A7F41061A9E60),
	   U64(0xFCB6BE43A9F2FE9B), U64(0x08DE8A1C7797DA9B), U64(0x8F9887E6078735A1), U64(0xB5B4071DBFC73A66),
	   U64(0x230E343DFBA08D33), U64(0x43ED7F5A0FAE657D), U64(0x3A88A0FBBCB05C63), U64(0x21874B8B4D2DBC4F),
	   U64(0x1BDEA12E35F6A8C9), U64(0x53C065C6C8E63528), U64(0xE34A1D250E7A8D6B), U64(0xD6B04D3B7651DD7E),
	   U64(0x5E90277E7CB39E2D), U64(0x2C046F22062DC67D), U64(0xB10BB459132D0A26), U64(0x3FA9DDFB67E2F199),
	   U64(0x0E09B88E1914F7AF), U64(0x10E8B35AF3EEAB37), U64(0x9EEDECA8E272B933), U64(0xD4C718BC4AE8AE5F),
	   U64(0x81536D601170FC20), U64(0x91B534F885818A06), U64(0xEC8177F83F900978), U64(0x190E714FADA5156E),
	   U64(0xB592BF39B0364963), U64(0x89C350C893AE7DC1), U64(0xAC042E70F8B383F2), U64(0xB49B52E587A1EE60),
	   U64(0xFB152FE3FF26DA89), U64(0x3E666E6F69AE2C15), U64(0x3B544EBE544C19F9), U64(0xE805A1E290CF2456),
	   U64(0x24B33C9D7ED25117), U64(0xE74733427B72F0C1), U64(0x0A804D18B7097475), U64(0x57E3306D881EDB4F),
	   U64(0x4AE7D6A36EB5DBCB), U64(0x2D8D5432157064C8), U64(0xD1E649DE1E7F268B), U64(0x8A328A1CEDFE552C),
	   U64(0x07A3AEC79624C7DA), U64(0x84547DDC3E203C94), U64(0x990A98FD5071D263), U64(0x1A4FF12616EEFC89),
	   U64(0xF6F7FD1431714200), U64(0x30C05B1BA332F41C), U64(0x8D2636B81555A786), U64(0x46C9FEB55D120902),
	   U64(0xCCEC0A73B49C9921), U64(0x4E9D2827355FC492), U64(0x19EBB029435DCB0F), U64(0x4659D2B743848A2C),
	   U64(0x963EF2C96B33BE31), U64(0x74F85198B05A2E7D), U64(0x5A0F544DD2B1FB18), U64(0x03727073C2E134B1),
	   U64(0xC7F6AA2DE59AEA61), U64(0x352787BAA0D7C22F), U64(0x9853EAB63B5E0B35), U64(0xABBDCDD7ED5C0860),
	   U64(0xCF05DAF5AC8D77B0), U64(0x49CAD48CEBF4A71E), U64(0x7A4C10EC2158C4A6), U64(0xD9E92AA246BF719E),
	   U64(0x13AE978D09FE5557), U64(0x730499AF921549FF), U64(0x4E4B705B92903BA4), U64(0xFF577222C14F0A3A),
	   U64(0x55B6344CF97AAFAE), U64(0xB862225B055B6960), U64(0xCAC09AFBDDD2CDB4), U64(0xDAF8E9829FE96B5F),
	   U64(0xB5FDFC5D3132C498), U64(0x310CB380DB6F7503), U64(0xE87FBB46217A360E), U64(0x2102AE466EBB1148),
	   U64(0xF8549E1A3AA5E00D), U64(0x07A69AFDCC42261A), U64(0xC4C118BFE78FEAAE), U64(0xF9F4892ED96BD438),
	   U64(0x1AF3DBE25D8F45DA), U64(0xF5B4B0B0D2DEEEB4), U64(0x962ACEEFA82E1C84), U64(0x046E3ECAAF453CE9),
	   U64(0xF05D129681949A4C), U64(0x964781CE734B3C84), U64(0x9C2ED44081CE5FBD), U64(0x522E23F3925E319E),
	   U64(0x177E00F9FC32F791), U64(0x2BC60A63A6F3B3F2), U64(0x222BBFAE61725606), U64(0x486289DDCC3D6780),
	   U64(0x7DC7785B8EFDFC80), U64(0x8AF38731C02BA980), U64(0x1FAB64EA29A2DDF7), U64(0xE4D9429322CD065A),
	   U64(0x9DA058C67844F20C), U64(0x24C0E332B70019B0), U64(0x233003B5A6CFE6AD), U64(0xD586BD01C5C217F6),
	   U64(0x5E5637885F29BC2B), U64(0x7EBA726D8C94094B), U64(0x0A56A5F0BFE39272), U64(0xD79476A84EE20D06),
	   U64(0x9E4C1269BAA4BF37), U64(0x17EFEE45B0DEE640), U64(0x1D95B0A5FCF90BC6), U64(0x93CBE0B699C2585D),
	   U64(0x65FA4F227A2B6D79), U64(0xD5F9E858292504D5), U64(0xC2B5A03F71471A6F), U64(0x59300222B4561E00),
	   U64(0xCE2F8642CA0712DC), U64(0x7CA9723FBB2E8988), U64(0x2785338347F2BA08), U64(0xC61BB3A141E50E8C),
	   U64(0x150F361DAB9DEC26), U64(0x9F6A419D382595F4), U64(0x64A53DC924FE7AC9), U64(0x142DE49FFF7A7C3D),
	   U64(0x0C335248857FA9E7), U64(0x0A9C32D5EAE45305), U64(0xE6C42178C4BBB92E), U64(0x71F1CE2490D20B07),
	   U64(0xF1BCC3D275AFE51A), U64(0xE728E8C83C334074), U64(0x96FBF83A12884624), U64(0x81A1549FD6573DA5),
	   U64(0x5FA7867CAF35E149), U64(0x56986E2EF3ED091B), U64(0x917F1DD5F8886C61), U64(0xD20D8C88C8FFE65F),
	   U64(0x31D71DCE64B2C310), U64(0xF165B587DF898190), U64(0xA57E6339DD2CF3A0), U64(0x1EF6E6DBB1961EC9),
	   U64(0x70CC73D90BC26E24), U64(0xE21A6B35DF0C3AD7), U64(0x003A93D8B2806962), U64(0x1C99DED33CB890A1),
	   U64(0xCF3145DE0ADD4289), U64(0xD0E4427A5514FB72), U64(0x77C621CC9FB3A483), U64(0x67A34DAC4356550B),
	   U64(0xF8D626AAAF278509),
};


/*	GENHABD::GENHABD
 *
 *	Generates the random bit arrays used to compute the hash.
 */
GENHABD::GENHABD(void) : ihabdRandom(0)
{
	/* WARNING! - the order of these initializations is critical to making Polyglot book
	   lookup work. Don't change it! */

	ahabdPiece[0][0] = 0;	// shut up the compiler warning about uninitialized array
	for (APC apc = apcPawn; apc <= apcKing; ++apc)
		for (CPC cpc = cpcWhite; cpc <= cpcBlack; ++cpc)
			for (int rank = 0; rank < rankMax; rank++)
				for (int file = 0; file < fileMax; file++)
					ahabdPiece[SQ(rank, file)][PC(cpc, apc)] = HabdRandom(rgen);

	HABD habdWhiteKing = HabdRandom(rgen);
	HABD habdWhiteQueen = HabdRandom(rgen);
	HABD habdBlackKing = HabdRandom(rgen);
	HABD habdBlackQueen = HabdRandom(rgen);
	ahabdCastle[0] = 0;
	for (int cs = 1; cs < CArray(ahabdCastle); cs++) {
		ahabdCastle[cs] = 0;
		if (cs & csWhiteKing)
			ahabdCastle[cs] ^= habdWhiteKing;
		if (cs & csWhiteQueen)
			ahabdCastle[cs] ^= habdWhiteQueen;
		if (cs & csBlackKing)
			ahabdCastle[cs] ^= habdBlackKing;
		if (cs & csBlackQueen)
			ahabdCastle[cs] ^= habdBlackQueen;
	}

	for (int file = 0; file < CArray(ahabdEnPassant); file++)
		ahabdEnPassant[file] = HabdRandom(rgen); 

	habdMove = HabdRandom(rgen);
}


HABD GENHABD::HabdRandom(mt19937_64& rgen)
{
	return ihabdRandom < CArray(ahabdRandom) ? ahabdRandom[ihabdRandom++] : rgen();
}


/*	GENHABD::HabdFromBd
 *
 *	Creates the initial hash value for a new board position.
 */
HABD GENHABD::HabdFromBd(const BD& bd) const
{
	/* pieces */

	HABD habd = 0ULL;
	for (SQ sq = 0; sq < sqMax; sq++) {
		PC pc = bd.PcFromSq(sq);
		if (pc.apc() != apcNull)
			habd ^= ahabdPiece[sq][pc];
		}

	/* castle state */

	habd ^= ahabdCastle[bd.csCur];

	/* en passant state */

	if (!bd.sqEnPassant.fIsNil())
		habd ^= ahabdEnPassant[bd.sqEnPassant.file()];

	/* current side to move */

	if (bd.cpcToMove == cpcBlack)
		habd ^= habdMove;

	return habd;
}


/*
 *
 *	mpbb
 *
 *	We keep bitboards for every square and every direction of squares that are
 *	attacked from that square.
 * 
 */


MPBB mpbb;

MPBB::MPBB(void)
{
	/* precompute all the attack bitboards for sliders, kings, and knights */

	for (int rank = 0; rank < rankMax; rank++)
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);

			/* slides */
			
			for (int drank = -1; drank <= 1; drank++)
				for (int dfile = -1; dfile <= 1; dfile++) {
					if (drank == 0 && dfile == 0)
						continue;
					DIR dir = DirFromDrankDfile(drank, dfile);
					for (int rankMove = rank + drank, fileMove = file + dfile;
							((rankMove | fileMove) & ~7) == 0; rankMove += drank, fileMove += dfile)
						mpsqdirbbSlide[sq][(int)dir] |= BB(SQ(rankMove, fileMove));
				}

			/* knights */

			BB bb(sq);
			BB bb1 = BbWest1(bb) | BbEast1(bb);
			BB bb2 = BbWest2(bb) | BbEast2(bb);
			mpsqbbKnight[sq] = BbNorth2(bb1) | BbSouth2(bb1) | BbNorth1(bb2) | BbSouth1(bb2);

			/* kings */

			bb1 = BbEast1(bb) | BbWest1(bb);
			mpsqbbKing[sq] = bb1 | BbNorth1(bb1 | bb) | BbSouth1(bb1 | bb);

			/* passed pawn alleys */

			if (in_range(rank, 1, rankMax - 2)) {
				BB bbNorth = mpsqdirbbSlide[sq][dirNorth];
				BB bbSouth = mpsqdirbbSlide[sq][dirSouth];
				mpsqbbPassedPawnAlley[sq-8][cpcWhite] = bbNorth | BbEast1(bbNorth) | BbWest1(bbNorth);
				mpsqbbPassedPawnAlley[sq-8][cpcBlack] = bbSouth | BbEast1(bbSouth) | BbWest1(bbSouth);
			}
		}
}


/*
 *
 *	BD class implementation
 * 
 *	The minimal board implementation, that implements non-move history
 *	board. That means this class is not able to perform certain chess
 *	stuff by itself, like detect en passant or some draw situations.
 *
 */


/* game phase, whiich is a running sum of the types of pieces on the board,
   which is handy for AI eval */
static const GPH mpapcgph[apcMax] = { gphNone, gphNone, gphMinor, gphMinor,
										   gphRook, gphQueen, gphNone };

BD::BD(void)
{
	SetEmpty();
	Validate();
}


void BD::SetEmpty(void) noexcept
{
	for (PC pc = 0; pc < pcMax; ++pc)
		mppcbb[pc] = bbNone;
	mpcpcbb[cpcWhite] = bbNone;
	mpcpcbb[cpcBlack] = bbNone;
	bbUnoccupied = bbAll;

	csCur = 0;
	cpcToMove = cpcWhite;
	sqEnPassant = sqNil;

	habd = 0L;
	gph = gphMax;
}


/*	BD::operator==
 *
 *	Compare two board states for equality. Boards are equal if all the pieces
 *	are in the same place, the castle states are the same, and the en passant
 *	state is the same.
 * 
 *	This is mainly used to detect 3-repeat draw situations. 
 */
bool BD::operator==(const BD& bd) const noexcept
{
	return habd == bd.habd;
}


bool BD::operator!=(const BD& bd) const noexcept
{
	return !(*this == bd);
}


/*	BD::SzInitFENPieces
 *
 *	Reads the pieces out a Forsyth-Edwards Notation string. The piece
 *	table should start at szFEN. Returns the character after the last
 *	piece character, which will probably be the space character.
 */
void BD::InitFENPieces(const char*& szFEN)
{
	SetEmpty();

	/* parse the line */

	int rank = rankMax - 1;
	SQ sq = SQ(rank, 0);
	const char* pch;
	for (pch = szFEN; *pch && !isspace(*pch); pch++) {
		switch (*pch) {
		case 'p': AddPieceFEN(sq++, PC(cpcBlack, apcPawn)); break;
		case 'r': AddPieceFEN(sq++, PC(cpcBlack, apcRook)); break;
		case 'n': AddPieceFEN(sq++, PC(cpcBlack, apcKnight)); break;
		case 'b': AddPieceFEN(sq++, PC(cpcBlack, apcBishop)); break;
		case 'q': AddPieceFEN(sq++, PC(cpcBlack, apcQueen)); break;
		case 'k': AddPieceFEN(sq++, PC(cpcBlack, apcKing)); break;
		case 'P': AddPieceFEN(sq++, PC(cpcWhite, apcPawn)); break;
		case 'R': AddPieceFEN(sq++, PC(cpcWhite, apcRook)); break;
		case 'N': AddPieceFEN(sq++, PC(cpcWhite, apcKnight)); break;
		case 'B': AddPieceFEN(sq++, PC(cpcWhite, apcBishop)); break;
		case 'Q': AddPieceFEN(sq++, PC(cpcWhite, apcQueen)); break;
		case 'K': AddPieceFEN(sq++, PC(cpcWhite, apcKing)); break;
		case '/': sq = SQ(--rank, 0);  break;
		case '1':
		case '2': 
		case '3': 
		case '4': 
		case '5': 
		case '6':
		case '7':
		case '8':
			sq += *pch - '0'; 
			break;
		default:
			/* TODO: illegal character in the string - report error */
			assert(false);
			return;
		}
	}
	szFEN = pch;
}


/*	BD::AddPieceFEN
 *
 *	Adds a piece from the FEN piece stream to the board. Makes sure both
 *	board mappings are correct. 
 * 
 *	Promoted  pawns are the real can of worms with this process.
 */
void BD::AddPieceFEN(SQ sq, PC pc)
{
	SetBB(pc, sq);
	AddApcToGph(pc.apc());
}


void BD::InitFENSideToMove(const char*& sz)
{
	SkipToNonSpace(sz);
	cpcToMove = cpcWhite;
	for (; *sz && *sz != ' '; sz++) {
		switch (*sz) {
		case 'w': break;
		case 'b': ToggleToMove(); break;
		default: goto Done;
		}
	}
Done:
	SkipToSpace(sz);
}


void BD::SkipToSpace(const char*& sz)
{
	while (*sz && !isspace(*sz))
		sz++;
}


void BD::SkipToNonSpace(const char*& sz)
{
	while (*sz && isspace(*sz))
		sz++;
}


void BD::InitFENCastle(const char*& sz)
{
	SkipToNonSpace(sz);
	ClearCsCur(cpcWhite, csKing | csQueen);
	ClearCsCur(cpcBlack, csKing | csQueen);
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case 'K': SetCsCur(cpcWhite, csKing); break;
		case 'Q': SetCsCur(cpcWhite, csQueen); break;
		case 'k': SetCsCur(cpcBlack, csKing); break;
		case 'q': SetCsCur(cpcBlack, csQueen); break;
		case '-': break;
		default: goto Done;
		}
	}
Done:
	SkipToSpace(sz);
}


void BD::InitFENEnPassant(const char*& sz)
{
	SkipToNonSpace(sz);
	SQ sq;
	for (; *sz && !isspace(*sz); sz++) {
		if (in_range(*sz, 'a', 'h'))
			sq.SetFile(*sz - 'a');
		else if (in_range(*sz, '1', '8'))
			sq.SetRank(*sz - '1');
		else if (*sz == '-')
			sq = sqNil;
	}
	SetEnPassant(sq);
}


/*	BD::MakeMvSq
 *
 *	Makes a partial move on the board, only updating the square,
 *	tpc arrays, and bitboards. On return, the MV is modified to include
 *	information needed to undo the move (i.e., saves state for captures,
 *	castling, en passant, and pawn promotion).
 */
void BD::MakeMvSq(MVE& mve) noexcept
{
	Validate();

	assert(!mve.fIsNil());
	SQ sqFrom = mve.sqFrom();
	SQ sqTo = mve.sqTo();
	PC pcFrom = mve.pcMove();
	CPC cpcFrom = pcFrom.cpc();

	/* store undo information in the mv */

	mve.SetCsEp(csCur, sqEnPassant);

	/* captures. if we're taking a rook, we can't castle to that rook */

	SQ sqTake = sqTo;
	if (pcFrom.apc() == apcPawn && sqTake == sqEnPassant)
		sqTake = SQ(sqTo.rank() ^ 1, sqEnPassant.file());
	if (!FIsEmpty(sqTake)) {
		APC apcTake = ApcFromSq(sqTake);
		mve.SetCapture(apcTake);
		ClearBB(PC(~cpcFrom, apcTake), sqTake);
		RemoveApcFromGph(apcTake);
		if (apcTake == apcRook) {
			if (sqTake == SQ(RankBackFromCpc(~cpcFrom), fileKingRook))
				ClearCsCur(~cpcFrom, csKing);
			else if (sqTake == SQ(RankBackFromCpc(~cpcFrom), fileQueenRook))
				ClearCsCur(~cpcFrom, csQueen);
		}
	}

	/* move the pieces */

	ClearBB(pcFrom, sqFrom);
	SetBB(pcFrom, sqTo);

	switch (pcFrom.apc()) {
	case apcPawn:
		/* save double-pawn moves for potential en passant and return */
		if (((sqFrom.rank() ^ sqTo.rank()) & 0x03) == 0x02) {
			SetEnPassant(SQ(sqTo.rank() ^ 1, sqTo.file()));
			goto Done;
		}
		if (sqTo.rank() == 0 || sqTo.rank() == 7) {
			/* pawn promotion on last rank */
			ClearBB(PC(cpcFrom, apcPawn), sqTo);
			SetBB(PC(cpcFrom, mve.apcPromote()), sqTo);
			RemoveApcFromGph(apcPawn);
			AddApcToGph(mve.apcPromote());
		} 
		break;

	case apcKing:
		ClearCsCur(cpcFrom, csKing|csQueen);
		if (sqTo.file() - sqFrom.file() > 1) { // king side
			ClearBB(PC(cpcFrom, apcRook), sqTo + 1);
			SetBB(PC(cpcFrom, apcRook), sqTo - 1);
		}
		else if (sqTo.file() - sqFrom.file() < -1) { // queen side 
			ClearBB(PC(cpcFrom, apcRook), sqTo - 2);
			SetBB(PC(cpcFrom, apcRook), sqTo + 1);
		}
		break;

	case apcRook:
		if (sqFrom == SQ(RankBackFromCpc(cpcFrom), fileQueenRook))
			ClearCsCur(cpcFrom, csQueen);
		else if (sqFrom == SQ(RankBackFromCpc(cpcFrom), fileKingRook))
			ClearCsCur(cpcFrom, csKing);
		break;

	default:
		break;
	}

	SetEnPassant(sqNil);
Done:
	ToggleToMove();
	Validate();
}


/*	BD::UndoMvSq
 *
 *	Undo the move made on the board
 */
void BD::UndoMvSq(MVE mve) noexcept
{
	if (mve.fIsNil()) {
		UndoMvNullSq(mve);
		return;
	}

	assert(FIsEmpty(mve.sqFrom()));
	CPC cpcMove = mve.cpcMove();

	/* restore castle and en passant state. */

	SetCsCur(mve.csPrev());
	if (mve.fEpPrev())
		SetEnPassant(SQ(RankToEpFromCpc(cpcMove), mve.fileEpPrev()));
	else
		SetEnPassant(sqNil);

	/* put piece back in source square, undoing any pawn promotion that might
	   have happened */

	if (mve.apcPromote() == apcNull)
		ClearBB(PC(cpcMove, mve.apcMove()), mve.sqTo());
	else {
		ClearBB(PC(cpcMove, mve.apcPromote()), mve.sqTo());
		RemoveApcFromGph(mve.apcPromote());
		AddApcToGph(apcPawn);
	}
	SetBB(mve.pcMove(), mve.sqFrom());

	/* if move was a capture, put the captured piece back on the board; otherwise
	   the destination square becomes empty */

	if (mve.apcCapture() != apcNull) {
		SQ sqTake = mve.sqTo();
		if (sqTake == sqEnPassant)
			sqTake = SQ(RankTakeEpFromCpc(cpcMove), sqEnPassant.file());
		SetBB(PC(~cpcMove, mve.apcCapture()), sqTake);
		AddApcToGph(mve.apcCapture());
	}

	/* undoing a castle means we need to undo the rook, too */

	if (mve.apcMove() == apcKing) {
		int dfile = mve.sqTo().file() - mve.sqFrom().file();
		if (dfile < -1) { /* queen side */
			ClearBB(PC(cpcMove, apcRook), mve.sqTo() + 1);
			SetBB(PC(cpcMove, apcRook), mve.sqTo() - 2);
		}
		else if (dfile > 1) { /* king side */
			ClearBB(PC(cpcMove, apcRook), mve.sqTo() - 1);
			SetBB(PC(cpcMove, apcRook), mve.sqTo() + 1);
		}
	}

	ToggleToMove();

	Validate();
}


/*	BD::MakeMvNullSq
 *
 *	Makes a null move, i.e., skips the current movers turn. This is not a legal
 *	move, of course, but it is used by some AI heuristics, so we cobble up enough
 *	of the idea to get it to work. Keeps all board state, bitboars, and checksums.
 * 
 *	The returned mv includes the information necessary to undo the move. We ignore
 *	the move on entry.
 */
void BD::MakeMvNullSq(MVE& mve) noexcept
{
	mve = mveNil;
	mve.SetPcMove(PC(cpcToMove, apcNull));
	mve.SetCsEp(csCur, sqEnPassant);
	SetEnPassant(sqNil);
	ToggleToMove();
	Validate();
}


/*	BD::UndoMvNullSq
 * 
 *	Undoes a null move made by MakeMvNullSq.
 */
void BD::UndoMvNullSq(MVE mve) noexcept
{
	assert(mve.fIsNil());
	if (mve.fEpPrev())
		SetEnPassant(SQ(RankToEpFromCpc(mve.cpcMove()), mve.fileEpPrev()));
	else
		SetEnPassant(sqNil);
	ToggleToMove();
	Validate();
}


/*	BD::GenMoves
 *
 *	Generates the legal moves for the current player who has the move.
 *	Optionally doesn't bother to remove moves that would leave the
 *	king in check.
 */
void BD::GenMoves(VMVE& vmve, GG gg) noexcept
{
	GenMoves(vmve, cpcToMove, gg);
}


/*	BD::GenMoves
 *
 *	Generates verified moves for the given board position,
 *	where verified means we make sure all moves do not leave our own 
 *	king in check
 */
void BD::GenMoves(VMVE& vmve, CPC cpcMove, GG gg) noexcept
{
	Validate();

	vmve.clear();
	if (GgType(gg) == ggQuiet) {
		GenPawnQuiet(vmve, cpcMove);
		GenNonPawn(vmve, cpcMove, bbUnoccupied);
		GenCastles(vmve, cpcMove);
	}
	else if (GgType(gg) == ggNoisy) {
		GenPawnNoisy(vmve, cpcMove);
		GenNonPawn(vmve, cpcMove, mpcpcbb[~cpcMove]);
	}
	else {
		assert(GgType(gg) == ggAll);
		GenPawnQuiet(vmve, cpcMove);
		GenPawnNoisy(vmve, cpcMove);
		GenNonPawn(vmve, cpcMove, ~mpcpcbb[cpcMove]);
		GenCastles(vmve, cpcMove);
		int cmv = CmvPseudo(cpcMove);
		assert(vmve.size() == cmv);
	}

	if (FGgLegal(gg))
		RemoveInCheckMoves(vmve, cpcMove);
}


/*	BD::RemoveInCheckMoves
 *
 *	Removes the invalid check moves from a pseudo move list.
 */
void BD::RemoveInCheckMoves(VMVE& vmve, CPC cpcMove) noexcept
{
 	unsigned imveDest = 0;
	for (MVE mve : vmve) {
		MakeMvSq(mve);
		if (!FInCheck(cpcMove))
			vmve[imveDest++] = mve;
		UndoMvSq(mve);
	}
	vmve.resize(imveDest);
}


/*	BD::FMvIsQuiescent
 *
 *	TODO: should move this into the AI
 */
bool BD::FMvIsQuiescent(MVE mve) const noexcept
{
	return !mve.fIsCapture() && mve.apcPromote() == apcNull;
}


/*	BD::FInCheck
 *
 *	Returns true if the given color's king is in check.
 */
bool BD::FInCheck(CPC cpc) noexcept
{
	CPC cpcBy = ~cpc;

	/* for sliding pieces, reverse the attack to originate in the king, which is
	   more efficient becuase we know there is only one king */

	BB bbKing = mppcbb[PC(cpc, apcKing)];
	SQ sqKing = bbKing.sqLow();
	assert(bbKing.csq() == 1);
	BB bbQueen = mppcbb[PC(cpcBy, apcQueen)];
	if (BbBishop1Attacked(sqKing) & (bbQueen | mppcbb[PC(cpcBy, apcBishop)]))
		return true;
	if (BbRook1Attacked(sqKing) & (bbQueen | mppcbb[PC(cpcBy, apcRook)]))
		return true;

	/* other pieces need regular attack checks */
	
	BB bbAttacks = BbKnightAttacked(mppcbb[PC(cpcBy, apcKnight)]) |
		BbPawnAttacked(mppcbb[PC(cpcBy, apcPawn)], cpcBy) |
		BbKingAttacked(mppcbb[PC(cpcBy, apcKing)]);

	return bbAttacks & bbKing;
}


/*	BD::FIsCheckMate
 *
 *	If cpc is in check, checks if we have a checkmate. This is a very slow operation,
 *	don't use it in time critical code.
 */
bool BD::FIsCheckMate(CPC cpc) noexcept
{
	VMVE vmve;
	GenMoves(vmve, cpcToMove, ggLegal);
	return vmve.cmve() == 0;
}


/*	BD::BbPawnAttacked
 *
 *	Returns bitboard of all squares all pawns attack.
 * 
 */
__forceinline BB BD::BbPawnAttacked(BB bbPawns, CPC cpcBy) const noexcept
{
	bbPawns = cpcBy == cpcWhite ? BbNorth1(bbPawns) : BbSouth1(bbPawns);
	BB bbPawnAttacked = BbEast1(bbPawns) | BbWest1(bbPawns);
	return bbPawnAttacked;
}


/*	BD::BbKnightAttacked
 *
 *	Returns a bitboard of all squares a knight attacks.
 */
__forceinline BB BD::BbKnightAttacked(BB bbKnights) const noexcept
{
	BB bb1 = BbWest1(bbKnights) | BbEast1(bbKnights);
	BB bb2 = BbWest2(bbKnights) | BbEast2(bbKnights);
	return BbNorth2(bb1) | BbSouth2(bb1) | BbNorth1(bb2) | BbSouth1(bb2);
}


/*	BD::BbFwdSlideAttacks
 *
 *	Returns a bitboard of squares a sliding piece (rook, bishop, queen) attacks
 *	in the direction given. Only works for forward moves (north, northwest,
 *	northeast, and east).
 * 
 *	A square is attacked if it is empty or is the first square along the attack 
 *	ray that has a piece on it (either self of opponent color). So it includes
 *	not only squares that can be moved to, but also pieces that are defended.
 */
__forceinline BB BD::BbFwdSlideAttacks(DIR dir, SQ sqFrom) const noexcept
{
	assert(dir == dirEast || dir == dirNorthWest || dir == dirNorth || dir == dirNorthEast);
	BB bbAttacks = mpbb.BbSlideTo(sqFrom, dir);
	BB bbBlockers = (bbAttacks - bbUnoccupied) | BB(1ULL<<63);
	return bbAttacks ^ mpbb.BbSlideTo(bbBlockers.sqLow(), dir);
}


/*	BD::BbRevSlideAttacks
 *
 *	Returns bitboard of squares a sliding piece attacks in the direction given.
 *	Only works for reverse moves (south, southwest, southeast, and west).
 */
__forceinline BB BD::BbRevSlideAttacks(DIR dir, SQ sqFrom) const noexcept
{
	assert(dir == dirWest || dir == dirSouthWest || dir == dirSouth || dir == dirSouthEast);
	BB bbAttacks = mpbb.BbSlideTo(sqFrom, dir);
	BB bbBlockers = (bbAttacks - bbUnoccupied) | BB(1);
	return bbAttacks ^ mpbb.BbSlideTo(bbBlockers.sqHigh(), dir);
}


__forceinline BB BD::BbBishop1Attacked(SQ sq) const noexcept
{
	return BbFwdSlideAttacks(dirNorthEast, sq) |
		BbFwdSlideAttacks(dirNorthWest, sq) |
		BbRevSlideAttacks(dirSouthEast, sq) |
		BbRevSlideAttacks(dirSouthWest, sq);
}


__forceinline BB BD::BbBishopAttacked(BB bbBishops) const noexcept
{
	BB bbAttacks = 0;
	for ( ; bbBishops; bbBishops.ClearLow())
		bbAttacks |= BbBishop1Attacked(bbBishops.sqLow());
	return bbAttacks;
}


__forceinline BB BD::BbRook1Attacked(SQ sq) const noexcept
{
	return BbFwdSlideAttacks(dirNorth, sq) |
		BbFwdSlideAttacks(dirEast, sq) |
		BbRevSlideAttacks(dirSouth, sq) |
		BbRevSlideAttacks(dirWest, sq);
}


__forceinline BB BD::BbRookAttacked(BB bbRooks) const noexcept
{
	BB bbAttacks = 0;
	for ( ; bbRooks; bbRooks.ClearLow())
		bbAttacks |= BbRook1Attacked(bbRooks.sqLow());
	return bbAttacks;
}


__forceinline BB BD::BbQueen1Attacked(SQ sq) const noexcept
{
	return BbFwdSlideAttacks(dirNorth, sq) |
		BbFwdSlideAttacks(dirEast, sq) |
		BbRevSlideAttacks(dirSouth, sq) |
		BbRevSlideAttacks(dirWest, sq) |
		BbFwdSlideAttacks(dirNorthEast, sq) |
		BbFwdSlideAttacks(dirNorthWest, sq) |
		BbRevSlideAttacks(dirSouthEast, sq) |
		BbRevSlideAttacks(dirSouthWest, sq);
}


__forceinline BB BD::BbQueenAttacked(BB bbQueens) const noexcept
{
	BB bbAttacks = 0;
	for ( ; bbQueens; bbQueens.ClearLow())
		bbAttacks |= BbQueen1Attacked(bbQueens.sqLow());
	return bbAttacks;
}


__forceinline BB BD::BbKingAttacked(BB bbKing) const noexcept
{
	return mpbb.BbKingTo(bbKing.sqLow());
}


/*	BD::ApcBbAttacked
 *
 *	Returns the piece type of the lowest valued piece if the given bitboard is attacked 
 *	by someone of the color cpcBy. Returns apcNull if no pieces are attacking.
 */
APC BD::ApcBbAttacked(BB bbAttacked, CPC cpcBy) const noexcept
{
	if (BbPawnAttacked(mppcbb[PC(cpcBy, apcPawn)], cpcBy) & bbAttacked)
		return apcPawn;
	if (BbKnightAttacked(mppcbb[PC(cpcBy, apcKnight)]) & bbAttacked)
		return apcKnight;
	if (BbBishopAttacked(mppcbb[PC(cpcBy, apcBishop)]) & bbAttacked)
		return apcBishop;
	if (BbRookAttacked(mppcbb[PC(cpcBy, apcRook)]) & bbAttacked)
		return apcRook;
	if (BbQueenAttacked(mppcbb[PC(cpcBy, apcQueen)]) & bbAttacked)
		return apcQueen;
	if (BbKingAttacked(mppcbb[PC(cpcBy, apcKing)]) & bbAttacked)
		return apcKing;
	return apcNull;
}


/*	BD::GenPiece
 *
 *	Generates all moves fromn bbPiece to legal destinations in bbTo, with
 *	offset dsq. Returns the bitboard of squares we moved to.
 */
__forceinline BB BD::GenPiece(VMVE& vmve, PC pcMove, BB bbPieceFrom, BB bbTo, int dsq) const noexcept
{
	bbTo &= BbShift(bbPieceFrom, dsq);
	for (BB bb = bbTo; bb; bb.ClearLow()) {
		SQ sqTo = bb.sqLow();
		vmve.push_back(MVE(sqTo - dsq, sqTo, pcMove));
	}
	return bbTo;
}


/*	BD::GenPiece
 *
 *	Generates all moves with the source square in bbPiece and the possible legal
 *	destination squares in bbTo.
 */
__forceinline void BD::GenPiece(VMVE& vmve, PC pcMove, SQ sqFrom, BB bbPieceTo, BB bbTo) const noexcept
{
	for (bbTo &= bbPieceTo; bbTo; bbTo.ClearLow())
		vmve.push_back(MVE(sqFrom, bbTo.sqLow(), pcMove));
}


/*	BD::GenPawnPromotions
 *
 *	For pawn moves that result in a promotion. The pawns on the 7th rank are in
 *	bbPawns, the squares we're checking for the destination are in bbTo (which
 *	will be empty for straight-ahead moves, or the enemy squares if on the diagonal),
 *	with the direction of travel in dsq. cpcMove is only used for generating the
 *	PC stored in the MVE.
 */
__declspec(noinline) void BD::GenPawnPromotions(VMVE& vmve, BB bbPawns, BB bbTo, int dsq, CPC cpcMove) const noexcept
{
	BB bb = BbShift(bbPawns, dsq) & bbTo;
	for ( ; bb; bb.ClearLow()) {
		SQ sqTo = bb.sqLow();
		MVE mve(sqTo - dsq, sqTo, PC(cpcMove, apcPawn));
		mve.SetApcPromote(apcQueen);
		vmve.push_back(mve);
		mve.SetApcPromote(apcRook);
		vmve.push_back(mve);
		mve.SetApcPromote(apcBishop);
		vmve.push_back(mve);
		mve.SetApcPromote(apcKnight);
		vmve.push_back(mve);
	}
}


/*	BD::GenPawnNoisy
 *
 *	Generatges all pawn capture and promotion moves
 */
__forceinline void BD::GenPawnNoisy(VMVE& vmve, CPC cpcMove) const noexcept
{
	BB bbPawns = mppcbb[PC(cpcMove, apcPawn)];
	int dsqFore = cpcMove == cpcWhite ? dsqNorth : dsqSouth;
	BB bbEnemy(mpcpcbb[~cpcMove]);

	BB bbPromotions = bbPawns & BbRankPrePromote(cpcMove);
	if (bbPromotions) {
		GenPawnPromotions(vmve, bbPromotions - bbFileH, bbEnemy, dsqFore+dsqEast, cpcMove);
		GenPawnPromotions(vmve, bbPromotions - bbFileA, bbEnemy, dsqFore+dsqWest, cpcMove);
		GenPawnPromotions(vmve, bbPromotions, bbUnoccupied, dsqFore, cpcMove);
		bbPawns -= bbPromotions;
	}
	GenPiece(vmve, PC(cpcMove, apcPawn), bbPawns - bbFileH, bbEnemy, dsqFore+dsqEast);
	GenPiece(vmve, PC(cpcMove, apcPawn), bbPawns - bbFileA, bbEnemy, dsqFore+dsqWest);

	if (!sqEnPassant.fIsNil()) {
		BB bbEnPassant(sqEnPassant);
		BB bbFrom = BbWest1(bbEnPassant, -dsqFore) | BbEast1(bbEnPassant, -dsqFore);
		for (bbFrom &= bbPawns; bbFrom; bbFrom.ClearLow())
			vmve.push_back(bbFrom.sqLow(), sqEnPassant, PC(cpcMove, apcPawn));
	}
}


/*	BD::GenPawnQuiet
 *
 *	Generates the pawn non-captures on the board, for the pawns of color cpcMove.
 */
__forceinline void BD::GenPawnQuiet(VMVE& vmve, CPC cpcMove) const noexcept
{	
	BB bbPawns = mppcbb[PC(cpcMove, apcPawn)] - BbRankPrePromote(cpcMove);
	int dsqFore = cpcMove == cpcWhite ? dsqNorth : dsqSouth;
	BB bbTo = GenPiece(vmve, PC(cpcMove, apcPawn), bbPawns, bbUnoccupied, dsqFore);
	GenPiece(vmve, PC(cpcMove, apcPawn), BbShift(bbTo, -dsqFore) & BbRankPawnsInit(cpcMove), bbUnoccupied, 2*dsqFore);
}


/*	BD::GenNonPawn
 *
 *	Generates all pieces (except pawns and castles) moves that can move into the bbTo 
 *	bitmask. Pass in bbUnoccupied to generate quiet moves, or the opponent bitmask for
 *	captures, or both for all moves.
 */
__forceinline void BD::GenNonPawn(VMVE& vmve, CPC cpcMove, BB bbTo) const noexcept
{
	/* generate knight moves */

	PC pc = PC(cpcMove, apcKnight);
	for (BB bb = mppcbb[pc]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		GenPiece(vmve, pc, sqFrom, mpbb.BbKnightTo(sqFrom), bbTo);
	}

	/* generate bishop moves */

	pc = PC(cpcMove, apcBishop);
	for (BB bb = mppcbb[pc]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		GenPiece(vmve, pc, sqFrom, BbBishop1Attacked(sqFrom), bbTo);
	}

	/* generate rook moves */

	pc = PC(cpcMove, apcRook);
	for (BB bb = mppcbb[pc]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		GenPiece(vmve, pc, sqFrom, BbRook1Attacked(sqFrom), bbTo);
	}

	/* generate queen moves */

	pc = PC(cpcMove, apcQueen);
	for (BB bb = mppcbb[pc]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		GenPiece(vmve, pc, sqFrom, BbQueen1Attacked(sqFrom), bbTo);
	}

	/* generate king moves */

	pc = PC(cpcMove, apcKing);
	BB bbKing = mppcbb[pc];
	assert(bbKing.csq() == 1);
	SQ sqFrom = bbKing.sqLow();
	GenPiece(vmve, pc, sqFrom, mpbb.BbKingTo(sqFrom), bbTo);
}


/*	BD::GenCastles
 *
 *	Generates the legal castle moves for the king at sq. Checks that the king is in
 *	check and intermediate squares are not under attack, checks for intermediate 
 *	squares are empty, but does not check the final king destination for in check.
 */
void BD::GenCastles(VMVE& vmve, CPC cpcMove) const noexcept
{
	BB bbKing = mppcbb[PC(cpcMove, apcKing)];
	SQ sqKing = bbKing.sqLow();
	if (FCanCastle(cpcMove, csKing) && !((BbEast1(bbKing) | BbEast2(bbKing)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | BbEast1(bbKing), ~cpcMove) == apcNull)
			vmve.push_back(sqKing, sqKing + 2*dsqEast, PC(cpcMove, apcKing));
	}
	if (FCanCastle(cpcMove, csQueen) && !((BbWest1(bbKing) | BbWest2(bbKing) | BbWest3(bbKing)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | BbWest1(bbKing), ~cpcMove) == apcNull) 
			vmve.push_back(sqKing, sqKing + 2*dsqWest, PC(cpcMove, apcKing));
	}
}


/*
 *
 *	Move counts. Streamlined versions of the movegen that don't actually save the move
 *
 */

int BD::CmvPseudo(CPC cpcMove) const noexcept
{
	return CmvPawns(cpcMove) + CmvPieces(cpcMove) + CmvCastles(cpcMove);
}

__forceinline int BD::CmvPawns(CPC cpcMove) const noexcept
{
	int cmv = 0;

	BB bbPawns = mppcbb[PC(cpcMove, apcPawn)];
	int dsqFore = (cpcMove == cpcWhite) ? dsqNorth : dsqSouth;
	BB bbEnemy = mpcpcbb[~cpcMove];

	/* promotions */

	BB bbPromotions = bbPawns & BbRankPrePromote(cpcMove);
	if (bbPromotions) {
		cmv += 4 * (BbEast1(bbPromotions, dsqFore) & bbEnemy).csq();
		cmv += 4 * (BbWest1(bbPromotions, dsqFore) & bbEnemy).csq();;
		cmv += 4 * (BbVertical(bbPromotions, dsqFore) & bbUnoccupied).csq();
		bbPawns -= bbPromotions;
	}
	
	/* captures, regular pawn moves, and double first moves */
	
	cmv += (BbEast1(bbPawns, dsqFore) & bbEnemy).csq();
	cmv += (BbWest1(bbPawns, dsqFore) & bbEnemy).csq();
	BB bbOne = BbVertical(bbPawns, dsqFore) & bbUnoccupied;
	cmv += bbOne.csq();
	cmv += (BbVertical(bbOne & BbRankPawnsFirst(cpcMove), dsqFore) & bbUnoccupied).csq();

	/* en passant */

	if (!sqEnPassant.fIsNil()) {
		BB bbEnPassant(sqEnPassant);
		BB bbFrom = BbWest1(bbEnPassant, -dsqFore) | BbEast1(bbEnPassant, -dsqFore);
		cmv += (bbFrom & bbPawns).csq();
	}

	return cmv;
}


__forceinline int BD::CmvPieces(CPC cpcMove) const noexcept
{
	int cmv = 0;
	BB bbTo = ~mpcpcbb[cpcMove];
	
	for (BB bb = mppcbb[PC(cpcMove, apcKnight)]; bb; bb.ClearLow())
		cmv += (mpbb.BbKnightTo(bb.sqLow()) & bbTo).csq();
	for (BB bb = mppcbb[PC(cpcMove, apcBishop)]; bb; bb.ClearLow())
		cmv += (BbBishop1Attacked(bb.sqLow()) & bbTo).csq();
	for (BB bb = mppcbb[PC(cpcMove, apcRook)]; bb; bb.ClearLow())
		cmv += (BbRook1Attacked(bb.sqLow()) & bbTo).csq();
	for (BB bb = mppcbb[PC(cpcMove, apcQueen)]; bb; bb.ClearLow())
		cmv += (BbQueen1Attacked(bb.sqLow()) & bbTo).csq();

	BB bbKing = mppcbb[PC(cpcMove, apcKing)];
	assert(bbKing.csq() == 1);
	SQ sqFrom = bbKing.sqLow();
	cmv += (mpbb.BbKingTo(sqFrom) & bbTo).csq();
	
	return cmv;
}


__forceinline int BD::CmvCastles(CPC cpcMove) const noexcept
{
	BB bbKing = mppcbb[PC(cpcMove, apcKing)];
	SQ sqKing = bbKing.sqLow();
	int cmv = 0;
	if (FCanCastle(cpcMove, csKing) && !((BbEast1(bbKing) | BbEast2(bbKing)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | BbEast1(bbKing), ~cpcMove) == apcNull)
			cmv++;
	}
	if (FCanCastle(cpcMove, csQueen) && !((BbWest1(bbKing) | BbWest2(bbKing) | BbWest3(bbKing)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | BbWest1(bbKing), ~cpcMove) == apcNull)
			cmv++;
	}
	return cmv;
}



EV BD::EvFromSq(SQ sq) const noexcept
{
	static const EV mpapcev[] = { 0, 100, 300, 300, 500, 900, 10000, -1 };
	return mpapcev[ApcFromSq(sq)];
}


/*	BD::EvTotalFromCpc
 *
 *	Piece value of the entire board for the given color
 */
EV BD::EvTotalFromCpc(CPC cpc) const noexcept
{
	EV ev = 0;
	for (APC apc = apcPawn; apc < apcMax; ++apc) {
		for (BB bb = mppcbb[PC(cpc, apc)]; bb; bb.ClearLow())
			ev += EvFromSq(bb.sqLow());
	}
	return ev;
}


void BD::ClearSq(SQ sq) noexcept
{
	if (FIsEmpty(sq))
		return;
	PC pc = PcFromSq(sq);
	ClearBB(pc, sq);
	RemoveApcFromGph(pc.apc());
	GuessEpAndCastle(sq);
	Validate();
}


void BD::SetSq(SQ sq, PC pc) noexcept
{
	ClearSq(sq);
	SetBB(pc, sq);
	AddApcToGph(pc.apc());
	GuessEpAndCastle(sq);
	Validate();
}


/*	BD::GuessEpAndCastle
 *
 *	Temporary function that tries to guess what the correct castle and en passant states
 *	should be after changing the board through a user piece placement. This should be
 *	a temporary hack until we get a UI for setting castle and en passant states, along
 *	with error checking for illegal states.
 */
void BD::GuessEpAndCastle(SQ sq) noexcept
{
	GuessCastle(cpcWhite, rank1);
	GuessCastle(cpcBlack, rank8);
	SetEnPassant(sqNil);
}


void BD::GuessCastle(CPC cpc, int rank) noexcept
{
	ClearCsCur(cpc, csKing);
	ClearCsCur(cpc, csQueen);
	if (PcFromSq(SQ(rank, fileE)) == PC(cpc, apcKing)) {
		if (PcFromSq(SQ(rank, fileA)) == PC(cpc, apcRook))
			SetCsCur(cpc, csQueen);
		if (PcFromSq(SQ(rank, fileH)) == PC(cpc, apcRook))
			SetCsCur(cpc, csKing);
	}
}


GPH BD::GphCompute(void) const noexcept
{
	GPH gph = gphMax;
	for (APC apc = apcPawn; apc < apcMax; ++apc) {
		gph = static_cast<GPH>(static_cast<int>(gph) -
							   mppcbb[PC(cpcWhite, apc)].csq() * static_cast<int>(mpapcgph[apc]) -
							   mppcbb[PC(cpcBlack, apc)].csq() * static_cast<int>(mpapcgph[apc]));
	}
	return gph;
}


void BD::RecomputeGph(void) noexcept
{
	gph = GphCompute();
}


void BD::AddApcToGph(APC apc) noexcept
{
	gph = static_cast<GPH>(static_cast<int>(gph) - static_cast<int>(mpapcgph[apc]));
}


void BD::RemoveApcFromGph(APC apc) noexcept
{
	gph = static_cast<GPH>(static_cast<int>(gph) + static_cast<int>(mpapcgph[apc]));
}


/*	BD::Validate
 *
 *	Checks the board state for internal consistency
 */
#ifndef NDEBUG
void BD::Validate(void) const noexcept
{
	if (!fValidate)
		return;

	/* there must be between 1 and 16 pieces of each color */
	assert(mpcpcbb[cpcWhite].csq() >= 0 && mpcpcbb[cpcWhite].csq() <= 16);
	assert(mpcpcbb[cpcBlack].csq() >= 0 && mpcpcbb[cpcBlack].csq() <= 16);
	/* no more than 8 pawns */
	assert(mppcbb[pcWhitePawn].csq() <= 8);
	assert(mppcbb[pcBlackPawn].csq() <= 8);
	/* one king */
	//assert(mpapcbb[pcWhiteKing].csq() == 1);
	//assert(mpapcbb[pcBlackKing].csq() == 1);
	/* union of black, white, and empty bitboards should be all squares */
	assert((mpcpcbb[cpcWhite] | mpcpcbb[cpcBlack] | bbUnoccupied) == bbAll);
	/* white, black, and empty are disjoint */
	assert((mpcpcbb[cpcWhite] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[cpcBlack] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[cpcWhite] & mpcpcbb[cpcBlack]) == bbNone);

	for (int rank = 0; rank < rankMax; rank++) {
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			if (!bbUnoccupied.fSet(sq))
				ValidateBB(PcFromSq(sq), sq);
		}
	}

	/* check for valid castle situations */

	if (csCur & csWhiteKing) {
		assert(mppcbb[pcWhiteKing].fSet(SQ(rankWhiteBack, fileKing)));
		assert(mppcbb[pcWhiteRook].fSet(SQ(rankWhiteBack, fileKingRook)));
	}
	if (csCur & csBlackKing) {
		assert(mppcbb[pcBlackKing].fSet(SQ(rankBlackBack, fileKing)));
		assert(mppcbb[pcBlackRook].fSet(SQ(rankBlackBack, fileKingRook)));
	}
	if (csCur & csWhiteQueen) {
		assert(mppcbb[pcWhiteKing].fSet(SQ(rankWhiteBack, fileKing)));
		assert(mppcbb[pcWhiteRook].fSet(SQ(rankWhiteBack, fileQueenRook)));
	}
	if (csCur & csBlackQueen) {
		assert(mppcbb[pcBlackKing].fSet(SQ(rankBlackBack, fileKing)));
		assert(mppcbb[pcBlackRook].fSet(SQ(rankBlackBack, fileQueenRook)));
	}

	/* check for valid en passant situations */

	assert(sqEnPassant.fValid());
	if (!sqEnPassant.fIsNil()) {
		if (sqEnPassant.rank() == rank3) {
			assert(cpcToMove == cpcBlack);
			assert(mppcbb[pcWhitePawn].fSet(SQ(rank4, sqEnPassant.file())));
		}
		else if (sqEnPassant.rank() == rank6) {
			assert(cpcToMove == cpcWhite);
			assert(mppcbb[pcBlackPawn].fSet(SQ(rank5, sqEnPassant.file())));
		}
		else {
			assert(false);
		}
	}

	/* game phase should be accurate */

	assert(gph == GphCompute());

	/* make sure hash is kept accurate */

	assert(habd == genhabd.HabdFromBd(*this));
}


void BD::ValidateBB(PC pcVal, SQ sq) const noexcept
{
	for (CPC cpc = cpcWhite; cpc <= cpcBlack; ++cpc)
		for (APC apc = apcPawn; apc < apcMax; ++apc) {
			if (PC(cpc, apc) == pcVal) {
				assert(mppcbb[PC(cpc, apc)].fSet(sq));
				assert(mpcpcbb[cpc].fSet(sq));
				assert(!bbUnoccupied.fSet(sq));
			}
			else {
				assert(!mppcbb[PC(cpc, apc)].fSet(sq));
			}
		}

}

#endif

/*
 *
 *	BDG  class implementation
 * 
 *	This is the implementation of the full game board class, which is
 *	the board along with move history.
 * 
 */


/*	BDG::BDG
 *	
 *	Constructor for the game board.
 */
BDG::BDG(void) noexcept : gs(gsPlaying), imveCurLast(-1), imvePawnOrTakeLast(-1)
{
}


/*	BDG::BDG
 *
 *	Constructor for initializing a board with a FEN board state string.
 */
BDG::BDG(const char* szFEN)
{
	InitGame();
	SetFen(szFEN);
}


void BDG::InitGame(void)
{
	vmveGame.clear();
	vmveGame.SetDimveFirst(cpcToMove == cpcBlack);
	imveCurLast = -1;
	imvePawnOrTakeLast = -1;
	SetGs(gsNotStarted);
}


const char BDG::szFENInit[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


const char* BDG::SetFen(const char* sz)
{
	if (sz == nullptr)
		sz = szFENInit;
	InitFENPieces(sz);
	InitFENSideToMove(sz);
	InitFENCastle(sz);
	InitFENEnPassant(sz);
	InitFENHalfmoveClock(sz);
	InitFENFullmoveCounter(sz);
	vmveGame.SetDimveFirst(cpcToMove == cpcBlack);
	return sz;
}


void BDG::InitFENHalfmoveClock(const char*& sz)
{
	SkipToNonSpace(sz);
	if (*sz == '-')
		sz++;
	else if (isdigit(*sz)) {
		int imv = *sz - '0';
		for (sz++; isdigit(*sz); sz++)
			imv = 10 * imv + *sz - '0';
		/* TODO: what should I do with this? */
	}
}


void BDG::InitFENFullmoveCounter(const char*& sz)
{
	SkipToNonSpace(sz);
	if (*sz == '-') 
		sz++;
	else if (isdigit(*sz)) {
		int nmv = *sz - '0';
		for (sz++; isdigit(*sz); sz++)
			nmv = 10*nmv + *sz - '0';
		/* TODO: is this right? */
		vmveGame.SetDimveFirst((nmv-1)*2 + (cpcToMove == cpcBlack));
	}
 }

void BDG::InitEpdProperties(const char*& sz, map<string, vector<EPDP>>& mpszvepdp)
{
	for (;;) {
		string oc = SzEpdOpcode(sz);
		if (oc.size() == 0)
			break;
		vector<EPDP> vepdp;
		ReadEpdOperands(sz, vepdp);
		if (vepdp.size() == 0)
			break;
		mpszvepdp[oc] = vepdp;
	}
}


string BDG::SzEpdOpcode(const char*& sz)
{
	string szOpcode;
	SkipToNonSpace(sz);
	for (; isalnum(*sz) || *sz == '_'; sz++)
		szOpcode += *sz;
	return szOpcode;
}


void BDG::ReadEpdOperands(const char*& sz, vector<EPDP>& vepdp)
{
	SkipToNonSpace(sz);
	EPDP epdp;
	while (*sz != ';') {
		if (!*sz)
			return;
		ReadEpdOperand(sz, epdp);
		vepdp.push_back(epdp);
	}
	sz++;
}


void BDG::ReadEpdOperand(const char*& sz, EPDP& epdp)
{
	SkipToNonSpace(sz);
	/* TODO: parse these as different types : string, move, unsigned, integer, float */
	if (*sz == '"') {
		string szVal;
		for (sz++; *sz != '"'; sz++) {
			if (!*sz)	// TODO: error reporting?
				return;
			szVal += *sz;
		}
		epdp.SetSz(szVal);
		sz++;
	}
	else {
		string szVal;
		while (*sz && !isspace(*sz) && *sz != ';')
			szVal += *sz++;
		epdp.SetSz(szVal);
	}
}


/*	BDG::SzFEN
 *
 *	Returns the FEN string of the given board
 */
string BDG::SzFEN(void) const
{
	string sz;

	/* write the squares and pieces */

	for (int rank = rankMax-1; rank >= 0; rank--) {
		int csqEmpty = 0;
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			if (FIsEmpty(sq))
				csqEmpty++;
			else {
				if (csqEmpty > 0) {
					sz += to_string(csqEmpty);
					csqEmpty = 0;
				}
				char ch = " PNBRQK"[ApcFromSq(sq)];
				if (CpcFromSq(sq) == cpcBlack)
					ch += 'a' - 'A';
				sz += ch;
			}
		}
		if (csqEmpty > 0)
			sz += to_string(csqEmpty);
		sz += '/';
	}

	/* piece to move */

	sz += ' ';
	sz += cpcToMove == cpcWhite ? 'w' : 'b';
	
	/* castling */

	sz += ' ';
	if (csCur == 0)
		sz += '-';
	else {
		if (csCur & csWhiteKing)
			sz += 'K';
		if (csCur & csWhiteQueen)
			sz += 'Q';
		if (csCur & csBlackKing)
			sz += 'k';
		if (csCur & csBlackQueen)
			sz += 'q';
	}

	/* en passant */

	sz += ' ';
	if (sqEnPassant.fIsNil())
		sz += '-';
	else {
		sz += ('a' + sqEnPassant.file());
		sz += ('1' + sqEnPassant.rank());
	}

	/* halfmove clock */

	sz += ' ';
	sz += to_string(imveCurLast - imvePawnOrTakeLast);

	/* fullmove number */

	sz += ' ';
	sz += to_string(1 + imveCurLast/2);

	return sz;
}


/*	BDG::SzMoveAndDecode
 *
 *	Makes a move and returns the decoded text of the move. This is the only
 *	way to get postfix marks on the move text, like '+' for check, or '#' for
 *	checkmate.
 */
wstring BDG::SzMoveAndDecode(MVE mve)
{
	wstring sz = SzDecodeMv(mve, true);
	CPC cpc = CpcFromSq(mve.sqFrom());
	MakeMv(mve);
	if (FInCheck(~cpc))
		sz += FIsCheckMate(~cpc) ? '#' : '+';
	return sz;
}


int BDG::NmvNextFromCpc(CPC cpc) const
{
	return vmveGame.NmvFromImv(vmveGame.size() + (cpcToMove != cpc));
}


/*	BDG::MakeMv
 *
 *	Make a move on the board, and keeps the move list for the game. Caller is
 *	responsible for testing for game over.
 */
void BDG::MakeMv(MVE& mve) noexcept
{
	assert(!mve.fIsNil());

	/* make the move and save the move in the move list */

	MakeMvSq(mve);
	if (++imveCurLast == vmveGame.size())
		vmveGame.push_back(mve);
	else {
		vmveGame[imveCurLast] = mve;
		vmveGame.resize(imveCurLast + 1);
	}

	/* keep track of last pawn move or capture, which is used to detect draws */

	if (mve.apcMove() == apcPawn || mve.fIsCapture() || mve.fIsCastle())
		imvePawnOrTakeLast = imveCurLast;
}


void BDG::MakeMvNull(void) noexcept
{
	MVE mve;
	MakeMvNullSq(mve);
	if (++imveCurLast == vmveGame.size())
		vmveGame.push_back(mve);
	else 
		vmveGame[imveCurLast] = mve;
}


/*	BDG::UndoMv
 *
 *	Undoes the last made move at imvCur. Caller is responsible for resetting game
 *	over state
 */
void BDG::UndoMv(void) noexcept
{
	if (imveCurLast < 0)
		return;
	if (imveCurLast == imvePawnOrTakeLast) {
		/* scan backwards looking for pawn moves or captures */
		for (imvePawnOrTakeLast = imveCurLast-1; imvePawnOrTakeLast >= 0; imvePawnOrTakeLast--)
			if (vmveGame[imvePawnOrTakeLast].apcMove() == apcPawn || 
					vmveGame[imvePawnOrTakeLast].fIsCapture() ||
					vmveGame[imvePawnOrTakeLast].fIsCastle())
				break;
	}
	UndoMvSq(vmveGame[imveCurLast--]);
	assert(imveCurLast >= -1);
}


/*	BDG::RedoMv
 *
 *	Redoes that last undone move, which will be at imvCur+1. Caller is responsible
 *	for testing for game-over state afterwards.
 */
void BDG::RedoMv(void) noexcept
{
	if (imveCurLast > vmveGame.size() || vmveGame[imveCurLast+1].fIsNil())
		return;
	imveCurLast++;
	MVE mve = vmveGame[imveCurLast];
	if (mve.apcMove() == apcPawn || mve.fIsCapture() || mve.fIsCastle())
		imvePawnOrTakeLast = imveCurLast;
	MakeMvSq(mve);
}


/*	BDG::GsTestGameOver
 *
 *	Tests for the game in an end state. Returns the new state. Takes the legal move
 *	list for the current to-move player and the rule struture.
 */
GS BDG::GsTestGameOver(int cmvToMove, int cmvRepeatDraw) noexcept
{
	if (cmvToMove == 0) {
		if (FInCheck(cpcToMove))
			return cpcToMove == cpcWhite ? gsWhiteCheckMated : gsBlackCheckMated;
		else
			return gsStaleMate;
	}
	else {
		/* check for draw circumstances */
		if (FDrawDead())
			return gsDrawDead;
		if (FDraw3Repeat(cmvRepeatDraw))
			return gsDraw3Repeat;
		if (FDraw50Move(50))
			return gsDraw50Move;
	}
	return gsPlaying;
}


void BDG::SetGameOver(const VMVE& vmve, const RULE& rule) noexcept
{
	SetGs(GsTestGameOver(vmve.cmve(), rule.CmvRepeatDraw()));
}


/*	BDG::FDrawDead
 *
 *	Returns true if we're in a board state where no one can force checkmate on the
 *	other player.
 */
bool BDG::FDrawDead(void) const noexcept
{
	/* any pawns, rooks, or queens means no draw */

	if (mppcbb[PC(cpcWhite, apcPawn)] || mppcbb[PC(cpcBlack, apcPawn)] ||
			mppcbb[PC(cpcWhite, apcRook)] || mppcbb[PC(cpcBlack, apcRook)] ||
			mppcbb[PC(cpcWhite, apcQueen)] || mppcbb[PC(cpcBlack, apcQueen)])
		return false;
	
	/* only bishops and knights on the board from here on out. everyone has a king */

	/* if either side has more than 2 pieces, no draw */

	if (mpcpcbb[cpcWhite].csq() > 2 || mpcpcbb[cpcBlack].csq() > 2)
		return false;

	/* K vs. K, K-N vs. K, or K-B vs. K is a draw */

	if (mpcpcbb[cpcWhite].csq() == 1 || mpcpcbb[cpcBlack].csq() == 1)
		return true;
	
	/* The other draw case is K-B vs. K-B with bishops on same color */

	if (mppcbb[pcWhiteBishop] && mppcbb[pcBlackBishop]) {
		SQ sqBishopBlack = mppcbb[pcBlackBishop].sqLow();
		SQ sqBishopWhite = mppcbb[pcWhiteBishop].sqLow();
		if (((sqBishopWhite ^ sqBishopBlack) & 1) == 0)
			return true;
	}

	/*  otherwise forcing checkmate is still possible */

	return false;
}


/*	BDG::FDraw3Repeat
 *
 *	Returns true on the draw condition when we've had the exact same board position
 *	for 3 times in a single game, where exact board position means same player to move,
 *	all pieces in the same place, castle state is the same, and en passant possibility
 *	is the same.
 * 
 *	cbdDraw = 0 means don't check for repeated position draws, always return false.
 */
bool BDG::FDraw3Repeat(int cbdDraw) const noexcept
{
	if (cbdDraw == 0)
		return false;
	if (imveCurLast - imvePawnOrTakeLast < (cbdDraw-1) * 2 * 2)
		return false;
	BD bd = *this;
	int cbdSame = 1;
	assert(imveCurLast - 1 >= 0);
	bd.UndoMvSq(vmveGame[imveCurLast]);
	bd.UndoMvSq(vmveGame[imveCurLast - 1]);
	for (int imve = imveCurLast - 2; imve >= imvePawnOrTakeLast + 2; imve -= 2) {
		bd.UndoMvSq(vmveGame[imve]);
		bd.UndoMvSq(vmveGame[imve - 1]);
		if (bd == *this) {
			if (++cbdSame >= cbdDraw)
				return true;
			imve -= 2;
			/* let's go ahead and back up two more moves here, since we can't possibly 
			   match the next position */
			if (imve < imvePawnOrTakeLast + 2)
				break;
			bd.UndoMvSq(vmveGame[imve]);
			bd.UndoMvSq(vmveGame[imve - 1]);
			assert(bd != *this);
		}
	}
	return false;
}


/*	BDG::FDraw50Move
 *
 *	If we've gone 50 moves (black and white both gone 50 moves each) without a pawn move
 *	or a capture. 
 */
bool BDG::FDraw50Move(int cmvDraw) const noexcept
{
	return imveCurLast - imvePawnOrTakeLast >= cmvDraw * 2;
}


/*	BDG::SetGs
 *
 *	Sets the game state.
 */
void BDG::SetGs(GS gs) noexcept
{
	this->gs = gs;
}


/*	SzFromEv
 *
 *	Creates a string from an evaluation. Evaluations are in centi-pawns, so
 *	this basically returns a string representing a number with 2-decimal places.
 * 
 *	Handles special evaluation values, like infinities, aborts, mates, etc.,
 *	so be aware that this doesn't always return just a simple numeric string.
 */
wstring SzFromEv(EV ev)
{
	wchar_t sz[20] = { 0 }, * pch = sz;
	if (ev >= 0)
		*pch++ = L'+';
	else {
		*pch++ = L'-';
		ev = -ev;
	}
	if (ev == evInf) {
		lstrcpy(pch, L"Inf");
		pch += lstrlen(pch);
	}
	else if (ev == evCanceled) {
		lstrcpy(pch, L"(interrupted)");
		pch += lstrlen(pch);
	}
	else if (ev == evTimedOut) {
		lstrcpy(pch, L"(timeout)");
		pch += lstrlen(pch);
	}
	else if (FEvIsMate(ev)) {
		*pch++ = L'#';
		pch = PchDecodeInt((DFromEvMate(ev) + 1) / 2, pch);
	}
	else if (ev > evInf)
		*pch++ = L'*';
	else {
		pch = PchDecodeInt(ev / 100, pch);
		ev %= 100;
		*pch++ = L'.';
		*pch++ = L'0' + ev / 10;
		*pch++ = L'0' + ev % 10;
	}
	*pch = 0;
	return wstring(sz);
}


wstring to_wstring(TSC tsc)
{
	switch (tsc) {
	case tscEvOther:
		return L"EV";
	case tscEvCapture:
		return L"CAP";
	case tscXTable:
		return L"XT";
	case tscPrincipalVar:
		return L"PV";
	default:
		return L"";
	}
}


wstring to_wstring(MV mv)
{
	if (mv.fIsNil())
		return L"--";
	wchar_t sz[8] = { 0 }, * pch = sz;
	*pch++ = L'a' + mv.sqFrom().file();
	*pch++ = L'1' + mv.sqFrom().rank();
	*pch++ = L'a' + mv.sqTo().file();
	*pch++ = L'1' + mv.sqTo().rank();
	if (mv.apcPromote() != apcNull) {
		*pch++ = L'=';
		*pch++ = L" PNBRQK"[mv.apcPromote()];
	}
	*pch = 0;
	return wstring(sz);
}


/*
 *
 *	RULE class
 * 
 *	Our one-shop stop for all the rule variants that we might have. Includes stuff
 *	like time control, repeat position counts, etc.
 * 
 */


RULE::RULE(void) : cmvRepeatDraw(3)
{
	vtmi.push_back(TMI(1, -1, msecMin * 30, msecSec * 3));	/* 30min and 3sec is TCEC early time control */
//	vtmi.push_back(TMI(1, 40, msecMin * 100, 0));
//	vtmi.push_back(TMI(41, 60, msecMin * 50, 0));
//	vtmi.push_back(TMI(61, -1, msecMin * 15, msecSec * 30));
}


/*	RULE::RULE
 *
 *	Create a new rule set with simple game/move increments and the number of repeated
 *	positions to trigger a draw.
 */
RULE::RULE(int dsecGame, int dsecMove, unsigned cmvRepeatDraw) : cmvRepeatDraw(cmvRepeatDraw)
{
	vtmi.push_back(TMI(1, -1, msecSec * dsecGame, msecSec * dsecMove));
}


void RULE::ClearTmi(void)
{
	vtmi.clear();
}

void RULE::AddTmi(int nmvFirst, int nmvLast, int dsecGame, int dsecMove)
{
	vtmi.push_back(TMI(nmvFirst, nmvLast, msecSec * dsecGame, msecSec * dsecMove));
}

int RULE::CtmiTotal(void) const
{
	return (int)vtmi.size();
}

const TMI& RULE::TmiFromNmv(int nmv) const
{
	assert(!FUntimed());
	for (vector<TMI>::const_iterator ptmi = vtmi.begin(); ptmi < vtmi.end(); ++ptmi) {
		if (ptmi->FContainsNmv(nmv))
			return *ptmi;
	}
	return vtmi.back();
}


const TMI& RULE::TmiFromItmi(int itmi) const
{
	assert(!FUntimed());
	return vtmi[itmi];
}


/*	RULE::FUntimed
 *
 *	Returns true if this is an untimed game.
 */
bool RULE::FUntimed(void) const
{
	return vtmi.size() == 0;
}


/*	RULE::DmsecAddBlock
 *
 *	If the current board state is at the transition between timing intervals, returns
 *	the amount of time to add to the CPC player's clock after move number nmv is made.
 *	
 *	WARNING! nmv is 1-based.
 */
DWORD RULE::DmsecAddBlock(CPC cpc, int nmv) const
{
	for (const TMI& tmi : vtmi) {
		if (tmi.nmvFirst == nmv)	
			return tmi.dmsec;
	}

	return 0;
}


/*	RULE::DmsecAddMove
 *
 *	After a player moves, returns the amount of time to modify the player's clock
 *	based on move bonus.
 */
DWORD RULE::DmsecAddMove(CPC cpc, int nmv) const
{
	for (const TMI& tmi : vtmi) {
		if (tmi.FContainsNmv(nmv))
			return tmi.dmsecMove;
	}
	return 0;
}


/*	RULE::CmvRepeatDraw
 *
 *	Number of repeated positions used for detecting draws. Typically 3.
 */
int RULE::CmvRepeatDraw(void) const
{
	return cmvRepeatDraw;
}


/*	RULE::SetGameTime
 *
 *	Sets the amount of time each player gets on their game clock. -1 for an
 *	untimed game.
 * 
 *	This is not a very good general purpose time control API. Should be replaced
 *	with something more generally useful.
 */
void RULE::SetGameTime(CPC cpc, DWORD dsec)
{
	vtmi.clear();
	if (dsec != -1)
		vtmi.push_back(TMI(1, -1, msecSec*dsec, 0));
}


wstring RULE::SzTimeControlTitle(void) const
{
	DWORD dmsecTotal = 0;
	for (const TMI& tmi : vtmi)
		dmsecTotal += tmi.dmsec + (tmi.nmvLast == -1 ? 40 : tmi.nmvLast - tmi.nmvFirst + 1) * tmi.dmsecMove;

	if (dmsecTotal <= 3 * 60 * 1000)
		return L"Bullet";
	if (dmsecTotal < 8 * 60 * 1000)
		return L"Blitz";
	if (dmsecTotal < 30 * 60 * 1000)
		return L"Rapid";
	if (dmsecTotal <= 60 * 60 * 1000)
		return L"Classical";
	return L"Tournament";
}

wstring RULE::SzTimeControlSummary(void) const
{
	vector<TMI>::const_iterator ptmi = vtmi.begin();
	wstring sz = SzSummaryFromTmi(*ptmi++);
	for ( ; ptmi < vtmi.end(); ++ptmi)
		sz += L", " + SzSummaryFromTmi(*ptmi);
	return sz;
}

wstring RULE::SzSummaryFromTmi(const TMI& tmi) const
{
	wstring sz;
	if (tmi.nmvLast != -1) {
		sz += to_wstring(tmi.nmvLast - tmi.nmvFirst + 1);
		sz += L"/";
	}
	if (tmi.dmsec >= 60 * 1000)
		sz += to_wstring(tmi.dmsec / 1000 / 60) + L"m";
	else
		sz += to_wstring(tmi.dmsec / 1000) + L"s";
	if (tmi.dmsecMove > 0) {
		sz += L"+";
		if (tmi.dmsecMove > 60 * 1000)
			sz += to_wstring(tmi.dmsecMove / 1000 / 60) + L"m";
		else
			sz += to_wstring(tmi.dmsecMove / 1000);
	}
	return sz;
}