#import "@preview/cades:0.3.1": qr-code

#set page(
  paper: "a4",
  flipped: true,
  margin: (top: 1.2cm, bottom: 1.2cm, left: 1.5cm, right: 1.5cm),
)

#set text(
  font: ("New Computer Modern", "Noto Sans CJK SC"),
  size: 9pt,
  lang: "zh",
)

#show heading: it => {
  set text(fill: black)
  set strong(delta: 100)
  it
}

#show heading.where(level: 1): it => {
  align(center)[
    #block(text(size: 24pt, weight: "bold", fill: black)[#it.body])
    #v(0.1em)
    #line(length: 100%, stroke: 1.2pt + black)
    #v(0.4em)
  ]
}

#show heading.where(level: 2): it => {
  v(0.4em)
  block(text(size: 12pt, weight: "bold", fill: black)[#it.body])
  v(0.2em)
}

#align(center)[
  #text(size: 24pt, weight: "bold", fill: black)[NFC 门禁使用指南]
  #v(0.1em)
  #line(length: 100%, stroke: 1.2pt + black)
  #v(0.4em)
  #text(size: 10pt, fill: gray)[刷卡即开门 · 断电不丢失 · 自助登记与注销]
]

#v(0.6em)

#grid(
  columns: (1fr, 1fr),
  column-gutter: 0.8cm,
  [
    = 正常开门

    + 把登记过的 IC 卡贴近读卡区（RC522 模块）。
    + 听到舵机转动、门锁打开后进门，#strong[4 秒] 后自动关门，别磨蹭。
    + 卡没登记的话门不会开。
  ],
  [
    = 加一张新卡

    + 拿一根公对公杜邦线，#strong[短接 D4 和 D2]，进入写卡模式。
    + 把新卡贴上去读一下，UID 就存进白名单了。
    + 已经存过的卡会自动忽略；最多存 #strong[20 张]。
    + 记得拔掉短接线，恢复正常开门。
  ],
)

#grid(
  columns: (1fr, 1fr),
  column-gutter: 0.8cm,
  [
    = 删掉一张卡

    + 用杜邦线 #strong[短接 D4 和 D3]，进入删卡模式。
    + 把要删的卡贴上去读一下，就从白名单里移除了。
    + 后面的卡会自动往前补空位。
    + 记得拔掉短接线，恢复正常开门。
  ],
  [
    = 小提醒

    + 舵机和 RC522 都要供电稳，否则容易抖动或读不出卡。
    + 只认卡 UID，不读扇区内容。
    + 加卡优先于删卡，别同时短接 D2 和 D3。
    + 默认参数：开 0° / 关 180°，开门保持 4 秒。
  ],
)

#v(0.8em)

#line(length: 100%, stroke: 0.6pt + gray)

#v(0.4em)

#text(size: 11pt, weight: "bold", fill: black)[想自己搭一个？]
#v(0.2em)
#text(size: 9pt)[项目仓库（含完整源码），两个地址哪个能用用哪个（近期可能改名）：]

#v(0.4em)

#grid(
  columns: (1fr, 1fr),
  column-gutter: 0.8cm,
  align(top)[
    #grid(
      columns: (auto, 1fr),
      column-gutter: 0.4cm,
      align(top)[
        #qr-code(
          "https://github.com/BlueQuantumx/nfc_guard",
          width: 2.6cm,
          background: white,
          error-correction: "M",
        )
      ],
      align(top)[
        #raw("https://github.com/BlueQuantumx/nfc_guard")
      ],
    )
  ],
  align(top)[
    #grid(
      columns: (auto, 1fr),
      column-gutter: 0.4cm,
      align(top)[
        #qr-code(
          "https://github.com/egrecho047/nfc_guard",
          width: 2.6cm,
          background: white,
          error-correction: "M",
        )
      ],
      align(top)[
        #raw("https://github.com/egrecho047/nfc_guard")
      ],
    )
  ],
)
