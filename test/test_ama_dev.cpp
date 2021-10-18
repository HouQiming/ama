/*
@ama
const path=require('path');
const jsism=require('cpp/jsism');
const sane_types=require('cpp/sane_types');
const sane_init=require('cpp/sane_init');
const sane_export=require('cpp/sane_export');
const move_operator=require('cpp/move_operator');
const unified_null=require('cpp/unified_null');
let nd_root=ParseCurrentFile({parse_indent_as_scope:1});
//console.log(JSON.stringify(nd_root,null,1));
nd_root
    .StripRedundantPrefixSpace()
    .then(require('auto_semicolon'))
    .then(sane_types.FixArrayTypes)
    .then(require('cpp/typing').DeduceAuto)
    .then(require('cpp/auto_paren'))
    .Save('.self.audit.cpp')
    .then(sane_types,{view:{to:.(JC::array_base<.(Node.MatchAny('TElement'))>)}})
    .then(sane_init)
    .then(sane_export)
    .then(move_operator)
    .then(unified_null)
    .then(jsism.EnableJSLambdaSyntax)
    .then(jsism.EnableJSON)
    .then(require('cpp/auto_decl'))
    .then(require('cpp/auto_header'))
    .Save('.audit.cpp');
//make the per-file script take the translated source
ParseCurrentFile=function(){return nd_root;}
*/
#ifndef _CHARSET_JCH_HPP
#define _CHARSET_JCH_HPP
#include <array>
#include "jc_array.h"
namespace ama {
    /*#pragma add("jc_files", "./charset.jc");*/
    static inline uint32_t isInCharSet(uint32_t[:] cset, uint32_t ch) {
        ch &= 0xffu
        if 1:
            test()
            test2()
        return cset[ch >> 5] >> (ch & 31u) & 1u
    }
    char const* SkipChars(char const* feed, uint32_t[:] cset);
    uint32_t[8]! CreateCharSet(char const* s);
};

#endif
