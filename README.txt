efg : effective flat LMNtal goal

+ 実行準備
 - 必要なツール: flex2.5.6以降, bison
 - このディレクトリで make を実行
 - compiler ディレクトリに実行パスを通す
 - compiler ディレクトリ内の efg.sh を実行可能にする

+ 実行方法
 - > efg.sh hogehoge.lmn  でそこに hogehoge.exe が生成される
 - > ./hogehoge.exe  で実行

+ 対象プログラム
 - 膜とガードを一切使わない LMNtal プログラム
 - ただし，','で並んだプロセスは自由リンク名の文脈を共有するが，'.'で文脈はリセットされる
   (  x(X),x(X). は OK だが x(X).x(X). は NG  )
   (  x(X),x(X). x(X),x(X). は OK  )
 - コメントは # によるラインコメントのみ
 - 日本語は対応していない

+ その他
 - 出力ソースコードが見たい場合は efg.sh を改変
 - 内部中間命令列が見たい場合は compiler/main.cpp を改変
 - input/base 下はサンプル，input/forslim は SLIM 用なので efg では動かない
 - 結果表示が適当なので(アトム名クォートやリストなど)注意
