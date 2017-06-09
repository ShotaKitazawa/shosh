# TODO

- alias コマンドが効かない
  - alias テーブルをシェル内部に持つため
- C-c が一回しか動かない
  - そもそもSignalを無効化
    - http://qiita.com/advent-calendar/2016/make_editor (コンソールで動くエディタを作る)

- 以下が動かない
```
if ((ret = execvp(argv[0], argv)) == 0) printf("Error\n");
```

# タスク

- 完: プロンプトを表示してユーザのコマンド入力を受け付ける．

- 完: 入力されたコマンドを解釈・実行する．

- 完: コマンドの実行が終わると再びプロンプトを表示する．

- 完: 環境変数の展開 (${HOGE} > hogehoge)
    - {} が無いとダメ

- パイプ実装

---

- C-{f,b,a,e}, 矢印左右 の実装

- .shoshrc の作成

- .shosh_history と C-{p,n}, 矢印上下 の実装
