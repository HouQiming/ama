'use strict';
let g_templates = [
	{from: nRef('i8'), to: nRef('int8_t')},
	{from: nRef('i16'), to: nRef('int16_t')},
	{from: nRef('i32'), to: nRef('int32_t')},
	{from: nRef('i64'), to: nRef('int64_t')},
	{from: nRef('iptr'), to: nRef('intptr_t')},
	{from: nRef('u8'), to: nRef('uint8_t')},
	{from: nRef('u16'), to: nRef('uint16_t')},
	{from: nRef('u32'), to: nRef('uint32_t')},
	{from: nRef('u64'), to: nRef('uint64_t')},
	{from: nRef('uptr'), to: nRef('uintptr_t')},
	{from: nRef('f16'), to: nRef('__half')},
	{from: nRef('f32'), to: nRef('float')},
	{from: nRef('f64'), to: nRef('double')},
]

function BidirTransform(nd_root, alt_templates, is_forward) {
	return nd_root.TranslateTemplates(g_templates, is_forward);
};

/*
#filter Short numerical type names for C++
Before:
```C++
f32 powi(f32 a, u32 p){
	return p == 0 ? 1.f : (p & 1 ? a : 1.f) * powi(a, p >> 1);
}
iptr addr_powi = (iptr)(void*)powi;
```
*/
function Translate(nd_root, alt_templates) {
	return BidirTransform(nd_root, alt_templates, 1);
}

function Untranslate(nd_root, alt_templates) {
	return BidirTransform(nd_root, alt_templates, 0);
}

Translate.inverse = Untranslate;
module.exports = Translate;
