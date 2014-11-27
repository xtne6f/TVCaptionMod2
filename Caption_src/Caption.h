﻿#ifndef __CAPTION_H__
#define __CAPTION_H__

#ifdef __cplusplus
extern "C" {
#endif

//DLLの初期化
//戻り値：エラーコード
//ERR_INIT:既に初期化されている, TRUE:成功
__declspec(dllexport)
DWORD WINAPI InitializeCP();

//DLLの初期化(ワイド文字版)
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI InitializeCPW();

//DLLの初期化(UTF-8 mark10als互換)
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI InitializeUNICODE();

//DLLの開放
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI UnInitializeCP();

//188バイトTS1パケット
//戻り値：エラーコード
//ERR_NOT_INIT:初期化されていない, ERR_CAN_NOT_ANALYZ, ERR_NOT_FIRST, ERR_INVALID_PACKET, FALSE:その他の失敗,
//ERR_NEED_NEXT_PACKET:解析中(処理は正常に完了),
//CHANGE_VERSION:字幕の構成が変わった,
//NO_ERR_TAG_INFO:字幕の構成を取得した(ただし前回から変更はない),
//NO_ERR_CAPTION_1～NO_ERR_CAPTION_8:第n言語の新しい字幕文が得られた(ただし運用規定上は第2言語まで),
//TRUE:字幕文を取得した(ただし前回から変更はない)
__declspec(dllexport)
DWORD WINAPI AddTSPacketCP(BYTE* pbPacket);

//内部データクリア
//戻り値：エラーコード
//ERR_NOT_INIT:初期化されていない, 0:クリアした
__declspec(dllexport)
DWORD WINAPI ClearCP();

//以下の関数で返されるバッファはつぎにAddTSPacketCP、UnInitializeCP、ClearCPのいずれかを呼ぶまで有効

//字幕情報取得
//戻り値：エラーコード
//ERR_NOT_INIT:初期化されていない, FALSE:失敗, TRUE:成功(*pdwListCount>=1)
__declspec(dllexport)
DWORD WINAPI GetTagInfoCP(LANG_TAG_INFO_DLL** ppList, DWORD* pdwListCount);

//字幕データ本文取得
//戻り値：エラーコード
//ERR_NOT_INIT:初期化されていない, FALSE:失敗, TRUE:成功(*pdwListCount>=1)
//現仕様ではpstCharListのpszDecodeがNULLになることはないが、元仕様では可能性があるのでNULLチェックするべき
__declspec(dllexport)
DWORD WINAPI GetCaptionDataCP(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount);

//字幕データ本文取得(ワイド文字版)
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI GetCaptionDataCPW(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount);

//DRCS図形取得
//戻り値：エラーコード
//ERR_NOT_INIT:初期化されていない, FALSE:失敗, TRUE:成功(*pdwListCount>=1)
__declspec(dllexport)
DWORD WINAPI GetDRCSPatternCP(unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount);

//外字変換テーブル設定
//戻り値：エラーコード
//ERR_NOT_INIT:初期化されていない, FALSE:失敗, TRUE:成功
//外字90～94,85,86区の変換テーブルをリセット(dwCommand==0)または設定(dwCommand==1)する
//成功すればpdwTableSizeに設定されたテーブルの要素数が返る
__declspec(dllexport)
DWORD WINAPI SetGaijiCP(DWORD dwCommand, const WCHAR* ppTable, DWORD* pdwTableSize);

//外字変換テーブル取得
//戻り値：エラーコード
//ERR_NOT_INIT:初期化されていない, FALSE:失敗, TRUE:成功
//外字90～94,85,86区の変換テーブルを取得(dwCommand==1)する
//成功すればpdwTableSizeに取得されたテーブルの要素数が返る
__declspec(dllexport)
DWORD WINAPI GetGaijiCP(DWORD dwCommand, WCHAR* ppTable, DWORD* pdwTableSize);

#ifdef __cplusplus
}
#endif

#endif