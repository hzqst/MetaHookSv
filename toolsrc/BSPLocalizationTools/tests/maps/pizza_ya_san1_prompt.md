## Goal

Translate GoldSrc game_text messages into the requested target language.
Preserve literal \n sequences, punctuation intensity, urgency, and proper nouns when appropriate.
Return natural in-game text, not explanatory notes.

If the given message has two parts (japanese romaji and english equivalent), then you should always translate the romaji part into it's hiragana/katakana form, and translate the english equivalent to target language (schinese for example).

## Example

"pizza, tabetai na ... \n(I wanna eat pizza... )"

should be translated to

"ピザ ,食べたいな...\n(好想吃披萨啊...)"

when target language is schinese.