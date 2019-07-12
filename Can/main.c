/***********************************************************************/
/*                                                                     */
/*  FILE        :Main.c                                                */
/*  DATE        :                                                      */
/*  DESCRIPTION :Main Program                                          */
/*  CPU TYPE    :                                                      */
/*                                                                     */
/*  NOTE:THIS IS A TYPICAL EXAMPLE.                                    */
/*                                                                     */
/***********************************************************************/
#include "iodefine.h"
#include "r_cg_timer.h"

void fn_Init_CAN(void)
{
    //CANポートの設定
    PIOR4 = 0x40; //周辺I/Oリダイレクションレジスタ設定
    P7 |= 0x04; //P72(CTXD)
    PM7 &= 0xFB; //CTXD0(P72pin)を"0":出力に
    //PMC7 &= 0xFB; //PMCの設定CAN使用時は"0"
    PM7 |= 0x08; //CRXD0(P73pin)を"1":入力に
    //PMC7 &= 0xF7; //PMCの設定CAN使用時は"0"

    //外付け水晶振動子 8MHz使用時----------
    CAN0EN = 1; //CANモジュールへクロックを供給
    CAN0MCKE = 1; //CANモジュールへX1クロックを供給
    __nop();

    while ((GSTS & 0x0008) != 0) {
    } //CAN用RAMクリアを待つ GRAMINITフラグが"0":RAMクリアになったか?
    GCTRL &= 0xFFFB; //GSLPR=0  グローバルリセットモードに変遷 (GMDC=01)
    while ((GSTS & 0x0004) != 0) {
    } //CANグローバルリセットモード変遷を確認
    C0CTRL &= 0xFFFB; //CSLPR=0 チャンネルストップモード⇒チャンネルリセットモードに変遷
    while ((C0STSL & 0x0004) != 0) {
    } //CANチャンネルリセットモードに変遷を確認
    GCFGL = 0x0010; //DCS=1:fCAN=X1=8MHzに設定 DLC配置禁止とは?
    C0CFGH = 0x0049; //SJW=1 TSEG2=5 TSEG1=10  に設定 ボーレートプリスケラ(1)分周無しの場合
    C0CFGL = 0x0000; //C0CFGL+1で分周=ボーレートプリスケラ = fCAN_8Mhz/((0+1)×16Tq)= 500Kbps

    //受信ルール設定
    // GAFLCFG  = 0x0006; //受信ルール数設定 0:ルール無し? or 1:受信バッファを使用する?
    GAFLCFG = 0x0001; // ルールNoが正解
    GRWCR = 0x0000; //受信ルール変更準備
    GAFLIDL0 = 0x0000; //受信ルール① ID設定  比較しない?
    GAFLIDH0 = 0x0000; //受信ルール② 標準ID・データフレーム
    GAFLML0 = 0x0000; //受信ルール③ 対応IDをマスクしない "0000"なので全bitチェックしないので全て受信する。
    GAFLMH0 = 0xE000; //受信ルール④ 標準IDか?とデータフレームか?他のCANノードが送信? を比較する。
    GAFLPL0 = 0x0001; //受信ルール⑤ 受信FIFOバッファ0(GAFLFDP0)をのみ選択する
    GAFLPH0 = 0x0000; //受信ルール⑥ DLCチェックしない
    GRWCR = 0x0001; //受信ルール変更完了

    //受信バッファ設定
    RFCC0 = 0xF302; //
    C0CTRL |= 0x0008; //RTBO=1:バスオフ強制復帰させる ?

    GCTRL &= 0xFFFC; //GSLPR=0 グローバルリセットモード⇒グローバル動作モードに変遷 (GMDC=00)
    while ((GSTS & 0x0001) != 0) {
    } //CANグローバル動作モード変遷を確認
    RFCC0 |= 0x0001; //RFE(1:FIFOバッファ使用許可) 注)グローバル動作モードに変更するp1264
    C0CTRL &= 0xFFFC; //CSLPR=0 チャンネルリセットモード⇒CANチャンネル通信モードに変遷 (CHMDC=00)
    while ((C0STSL & 0x0001) != 0) {
    } ///CANチャンネル通信モードに変遷を確認

    __nop();
}

int f_TRD0 = 0;
void user_main(void)
{
    unsigned char CAN_RX_DATA[8];
    unsigned char CAN_RX_DLC = 0;
    unsigned short CAN_RX_ID = 0;
    unsigned short i = 0;

    fn_Init_CAN();
    R_TAU0_Channel0_Start();

    while (1) {
        if (f_TRD0 != 0) { //1ms毎のイベント(TRD0-コンペアマッチ)

            f_TRD0 = 0;
            i++;
            if (i >= 1000) { //1s毎のタイミング
                i = 0;

                //★★★CANの送信(1秒毎の定期送信)★★★★★★★★★★★★★★★
                TMSTS0 &= 0xF9; //送信結果フラグクリア
                while ((TMSTS0 & 0x06) != 0) {
                } //確認
                TMIDH0 = 0x0000; //標準ID データフレーム 履歴をバッファしない
                // IDセット
                TMIDL0 = 0x0123; //送信IDバッファ0にIDをSET!

                // データ長セット
                TMPTR0 = 0x8000; //送信DLCバッファ0にDLCをSET!

                // データセット
                TMDF00L = CAN_RX_ID >> 8; //受信IDを送信
                TMDF00H = (unsigned char)(CAN_RX_ID & 0x00FF);
                TMDF10L = CAN_RX_DLC; //受信DLCを送信
                TMDF10H = CAN_RX_DATA[0]; //受信DATA0を送信
                TMDF20L = CAN_RX_DATA[1]; //受信DATA1を送信
                TMDF20H = CAN_RX_DATA[2]; //受信DATA2を送信
                TMDF30L = CAN_RX_DATA[3]; //受信DATA3を送信
                TMDF30H = 0x12; //固定データを送信
                TMC0 |= 0x01; //送信要求 TMTRを"1"に

                //★★★CANの受信★★★★★★★★★★★★★★★★★★★★★★★★★
                CAN_RX_ID = RFIDL0; //IDを格納
                CAN_RX_DLC = RFPTR0 >> 12; //DLCを格納
                CAN_RX_DATA[0] = RFDF00L; //Data0を格納
                CAN_RX_DATA[1] = RFDF00H; //Data1を格納
                CAN_RX_DATA[2] = RFDF10L; //Data2を格納
                CAN_RX_DATA[3] = RFDF10H; //Data3を格納
                CAN_RX_DATA[4] = RFDF20L; //Data4を格納
                CAN_RX_DATA[5] = RFDF20H; //Data5を格納
                CAN_RX_DATA[6] = RFDF30L; //Data6を格納
                CAN_RX_DATA[7] = RFDF30H; //Data7を格納
                RFPCTR0 = 0x00FF; //バッファポインターインクリメント
            }
        }
    }
}
