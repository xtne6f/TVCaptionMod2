TVTest TVCaptionMod2 Plugin ver.0.1 (人柱版) + Caption.dll改造版の改造版

■概要
odaruさんが公開された「字幕 Plugin For TVTest(20081216)」をベースに、mark10alsさ
んによるCaption2Ass_PCR付属のCaption.dll改造版の改変点を選択的に取りこみ、表示ま
わりを「字幕 Plugin For TVTest(20081216) 改変版(090627)」を参考にして作成した字
幕プラグインです。字幕の中央表示をしたいがために作りました。

■動作環境
・Windows XP以降。ただしVistaは未確認
・TVTest ver.0.7.6(たぶん) 以降
・通常版: Visual C++ 2005 SP1 再頒布可能パッケージ(TVTestが起動するなら入ってる)
・x64版:  Visual C++ 2010 SP1 再頒布可能パッケージ (x64)

■使い方
TVTestのPluginsフォルダにTVCaptionMod2.tvtpとCaption.dllとを入れ、右クリックメニ
ューからプラグインを有効にして、字幕のある番組で字幕が表示されればOKです。
TVCaptionMod2(x64).tvtpはx64版のTVTest利用者向けです。

■既知の不具合
たぶん色々な字幕形式に未対応です。以下思いつくもの:
・縦書き (なかなか遭遇しないのでどう表示されるかもわからない)
・ワンセグ字幕
・字幕スーパー
・点滅
・PNG形式の字幕

■設定ファイルについて
設定ファイル"TVCaptionMod2.ini"は初回使用時Pluginsフォルダに自動作成します。必要
な場合はこれを直接編集してカスタマイズしてください。

Version
    設定ファイルのバージョン
    # デフォルト値を出力するために使います。特にユーザがいじる必要はありません。
CaptionDll
    "Caption.dll"の場所を絶対パスかTVTestフォルダからの相対パスで指定
    # デフォルトは[=Plugins\Caption.dll]
    # 同時に利用する他のプラグインが使うファイルと同じものを指定してはいけない
SettingsIndex
    現在選択中の表示設定の番号
    # このキーより下の設定は実行中に切り替え(TVTest設定→キー割り当て→表示設定
    # 切り替え)できます。つまり↓のような感じです:
    # 
    # [Settings]
    # Version=1
    # CaptionDll=Plugins\Caption.dll
    # SettingsIndex=1
    # FaceName=Windows TV ゴシック
    # GaijiTableName=!typebank
    # Method=2
    # DelayTime=450
    # TextColor=-1
    # BackColor=-1
    # TextOpacity=-1
    # BackOpacity=-1
    # StrokeWidth=-6
    # StrokeSmoothLevel=1
    # Centering=0
    # 
    # [Settings1]
    # FaceName=和田研中丸ゴシック2004ARIB
    # GaijiTableName=!std
    # Method=2
    # DelayTime=450
    # TextColor=255255000
    # BackColor=000000255
    # TextOpacity=-1
    # BackOpacity=20
    # StrokeWidth=-6
    # StrokeSmoothLevel=5
    # Centering=1
    # 
    # [Settings2]
    # ...
FaceName
    使用するフォント名を指定
    # 何も指定しないと適当な等幅フォントが使われます。
    # ARIB外字が収録されているフォントを使用すると、記号も化けずに表示できます。
GaijiTableName
    使用する外字テーブル名を指定
    # まず、添付の"gaiji.zip"にある "{プラグイン名}_Gaiji_{テーブル名}.txt" から
    # フォントに合ったもの(メモ帳などでこのファイルを開けば、フォントに合ってい
    # れば記号が羅列されるので大体わかります)をえらび、Pluginsフォルダに置いてく
    # ださい。{テーブル名}の部分を指定することでその外字テーブルが使われます。
    # 
    # ARIB STD-B24第一分冊の表7-19および表7-20に忠実なフォントは[=!std]と指定し
    # てください。
    # 「Windows TV ゴシック/丸ゴシック/太丸ゴシック」についてはプラグインに組み
    # 込まれています。[=!typebank]と指定してください。
Method
    字幕の表示方法
    # いまのところ通常ウィンドウ[=1]かレイヤードウィンドウ[=2]のみです。
DelayTime
    字幕を受け取ってから表示するまでの遅延時間をミリ秒で指定
    # [=0]から[=5000]まで
TextColor / BackColor
    字幕文/背景枠の色
    # [=-1]のときは自動(TSに含まれる情報をそのまま使う)です。RGB値を10進数で表現
    # してください: (例) (R,G,B)=(128,64,0)のとき、[=128064000]
TextOpacity / BackOpacity
    字幕文/背景枠の透過率
    # [=-1]のときは自動(TSに含まれる情報をそのまま使う)です。0%[=0]～100%[=100]
    # で指定してください。
StrokeWidth
    字幕文の縁取りの幅
    # [=0]以上のときは画面の大きさにかかわらず指定した幅で縁取ります。
    # [=0]より小さいときは画面に応じて縁取りの幅を調整します。
StrokeSmoothLevel
    縁取りをなだらかに透過させる度合い
Centering
    字幕を画面中央に表示する[=1]かどうか
    # 厳密には字幕の表示領域を縦横2/3にして上部中央に配置します。

■ソースについて
Caption.dllのソースコードはodaruさんのもの、またはCaption2Ass_PCR付属のものとな
るべく差分をとれるように改変してます(WinMergeなどつかうとよいかも)。
TVCaptionMod2.tvtpのほうは基本的にスクラッチです。

TSファイルの解析のために、tsselect-0.1.8(
http://www.marumo.ne.jp/junk/tsselect-0.1.8.lzh)よりソースコードを改変利用してい
ます。
また、TVTest ver.0.7.23から"PseudoOSD.cpp"、"PseudoOSD.h"、ほかソースコードを改
変利用しています(ソースコメントに流用元を記述しています)。
また、Caption.dll改造版より多くを参考にしているので、このプラグインもCaption.dll
オリジナル版の以下ライセンスに従います。

------引用開始------
●EpgDataCap_Bon、TSEpgView_Sample、NetworkRemocon、Caption、TSEpgViewServerの
ソースの取り扱いについて
　特にGPLとかにはしないので自由に改変してもらって構わないです。
　改変して公開する場合は改変部分のソースぐらいは一緒に公開してください。
　（強制ではないので別に公開しなくてもいいです）
　EpgDataCap.dllの使い方の参考にしてもらうといいかも。

●EpgDataCap.dll、Caption.dllの取り扱いについて
　フリーソフトに組み込む場合は特に制限は設けません。ただし、dllはオリジナルのまま
　組み込んでください。
　このdllを使用したことによって発生した問題について保証は一切行いません。
　商用、シェアウェアなどに組み込むのは不可です。
------引用終了------
