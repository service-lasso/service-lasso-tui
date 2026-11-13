// Duplicate rejection is required before constructing objects at every scoped boundary.
export function parseStrictJSON(input) {
  if (typeof input !== "string" && input.length >= 3 && input[0] === 0xef && input[1] === 0xbb && input[2] === 0xbf) throw new Error("JSON BOM denied");
  const text = typeof input === "string" ? input : new TextDecoder("utf-8", { fatal: true }).decode(input);
  if (text.charCodeAt(0) === 0xfeff) throw new Error("JSON BOM denied");
  if (Buffer.byteLength(text) > 1048576) throw new Error("JSON quota denied");
  let offset = 0;
  const deny = () => { throw new Error("closed JSON denied"); };
  const space = () => { while (/[\t\n\r ]/u.test(text[offset] ?? "!") ) offset++; };
  const string = () => {
    if (text[offset] !== '"') deny();
    const start = offset++;
    while (offset < text.length) {
      const ch = text[offset++];
      if (ch === '"') return JSON.parse(text.slice(start, offset));
      if (ch === "\\") offset++;
    }
    deny();
  };
  const value = depth => {
    if (depth > 64) deny(); space();
    if (text[offset] === '{') {
      offset++; space(); const keys = new Set();
      if (text[offset] === '}') { offset++; return; }
      for (;;) {
        space(); const key = string(); if (keys.has(key)) deny(); keys.add(key);
        space(); if (text[offset++] !== ':') deny(); value(depth + 1); space();
        const delimiter = text[offset++]; if (delimiter === '}') return; if (delimiter !== ',') deny();
      }
    }
    if (text[offset] === '[') {
      offset++; space(); if (text[offset] === ']') { offset++; return; }
      for (;;) { value(depth + 1); space(); const delimiter = text[offset++]; if (delimiter === ']') return; if (delimiter !== ',') deny(); }
    }
    if (text[offset] === '"') { string(); return; }
    const match = /^(?:true|false|null|-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)/u.exec(text.slice(offset));
    if (!match) deny(); offset += match[0].length;
  };
  value(0); space(); if (offset !== text.length) deny();
  return JSON.parse(text);
}
