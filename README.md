# TODO

- 子プロセス実行時、C-z が効かない
  - どのように job 管理を実装するか

- pipe 入力時、2つ目のコマンドが終わらない

```
cat README.md | grep TODO
flag: 1
# TODO
    - TODO: {} が無いとダメ
(実行が終了しない)
```

# タスク

- 完: プロンプトを表示してユーザのコマンド入力を受け付ける．

- 完: 入力されたコマンドを解釈・実行する．

- 完: コマンドの実行が終わると再びプロンプトを表示する．

- 完: 環境変数の展開 (${HOGE} > hogehoge)
    - TODO: {} が無いとダメ

- パイプ実装
  - pipe 時の2つ目のコマンドが終わらない
    - 2つ以上のpipeに未対応

- メモリのfree

---

- 完: C-c による入力のやりなおし

- ~ (チルダ) の実装
  - ~ > $HOME
  - ~kitazawa > kitazawa の $HOME

- C-{f,b,a,e}
  - 矢印左右

- .shoshrc の作成

- .shosh_history と C-{p,n}
  -  矢印上下

- $(hoge) or `hoge` によるコマンド実行
    - fork して raw モードで実行、実行結果を親プロセスへ渡す
      - その間親プロセスを wait

- '*' の実装
  - hoge.* で hoge.txt, hoge.md ...

- alias コマンド用テーブルの実装
  - alias テーブルをシェル内部に持つため

- 内部コマンドの実装
  - exec コマンド
  - if や for など
  - jobs コマンド等、ジョブ管理の仕組

- Tab 補完

- main 関数が引数を取れるように修正
  - ./shosh tmp でシェルスクリプトを実行可能にする
