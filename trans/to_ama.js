const amacpp=require('cpp/amacpp');
const path=require('path');
ParseCurrentFile().then(amacpp.inverse).Save('.ama' + path.extname(__filename));
//ignore the per-file script
return;
