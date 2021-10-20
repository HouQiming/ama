const amacpp=require('amacpp');
ParseCurrentFile().then(amacpp.inverse).Save('.ama' + path.extname(__filename));
//ignore the per-file script
return;
