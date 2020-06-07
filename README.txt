efg : effective flat LMNtal goal

+ 実行準備
 - 必要なツール: flex2.5.6以降, bison

+ ビルド
 - autoreconf
 - automake --add-missing
 - autoreconf
 - ./configure
 - make

+ 実行方法
 - > compiler/efg.sh hogehoge.lmn  でそこに hogehoge.exe が生成される
 - > ./hogehoge.exe  で実行

+ 対象プログラム
 - 膜とガードを一切使わない LMNtal プログラム
 - ただし，','で並んだプロセスは自由リンク名の文脈を共有するが，'.'で文脈はリセットされる
   (  x(X),x(X). は OK だが x(X).x(X). は NG  )
   (  x(X),x(X). x(X),x(X). は OK  )
 - コメントは # によるラインコメントのみ
 - ガードが無いためプロセスコンテキストに型を付ける場合は $p:int のようにする
 - 日本語は対応していない

+ その他
 - 出力ソースコードが見たい場合は compiler/efg-bin hogehoge.lmn
 - 内部中間命令列が見たい場合は compiler/efg-bin -c hogehoge.lmn 
 - sample/ 下はサンプル
