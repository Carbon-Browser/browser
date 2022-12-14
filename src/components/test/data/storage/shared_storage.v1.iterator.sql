-- components_unittests --gtest_filter=SharedStorageDatabaseTest.Version1_LoadFromFile
--
-- .dump of a version 1 Shared Storage database.
BEGIN TRANSACTION;
CREATE TABLE meta(key LONGVARCHAR NOT NULL UNIQUE PRIMARY KEY, value LONGVARCHAR);
INSERT INTO "meta" VALUES('version','1');
INSERT INTO "meta" VALUES('last_compatible_version','1');
CREATE TABLE values_mapping(context_origin TEXT NOT NULL,key TEXT NOT NULL,value TEXT,PRIMARY KEY(context_origin,key)) WITHOUT ROWID;
INSERT INTO "values_mapping" VALUES ('http://google.com','key1','value1');
INSERT INTO "values_mapping" VALUES ('http://google.com','key2','value2');
INSERT INTO "values_mapping" VALUES ('http://google.com','key3','value3');
INSERT INTO "values_mapping" VALUES ('http://google.com','key4','value4');
INSERT INTO "values_mapping" VALUES ('http://google.com','key5','value5');
INSERT INTO "values_mapping" VALUES ('http://google.com','key6','value6');
INSERT INTO "values_mapping" VALUES ('http://google.com','key7','value7');
INSERT INTO "values_mapping" VALUES ('http://google.com','key8','value8');
INSERT INTO "values_mapping" VALUES ('http://google.com','key9','value9');
INSERT INTO "values_mapping" VALUES ('http://google.com','key10','value10');
INSERT INTO "values_mapping" VALUES ('http://google.com','key11','value11');
INSERT INTO "values_mapping" VALUES ('http://google.com','key12','value12');
INSERT INTO "values_mapping" VALUES ('http://google.com','key13','value13');
INSERT INTO "values_mapping" VALUES ('http://google.com','key14','value14');
INSERT INTO "values_mapping" VALUES ('http://google.com','key15','value15');
INSERT INTO "values_mapping" VALUES ('http://google.com','key16','value16');
INSERT INTO "values_mapping" VALUES ('http://google.com','key17','value17');
INSERT INTO "values_mapping" VALUES ('http://google.com','key18','value18');
INSERT INTO "values_mapping" VALUES ('http://google.com','key19','value19');
INSERT INTO "values_mapping" VALUES ('http://google.com','key20','value20');
INSERT INTO "values_mapping" VALUES ('http://google.com','key21','value21');
INSERT INTO "values_mapping" VALUES ('http://google.com','key22','value22');
INSERT INTO "values_mapping" VALUES ('http://google.com','key23','value23');
INSERT INTO "values_mapping" VALUES ('http://google.com','key24','value24');
INSERT INTO "values_mapping" VALUES ('http://google.com','key25','value25');
INSERT INTO "values_mapping" VALUES ('http://google.com','key26','value26');
INSERT INTO "values_mapping" VALUES ('http://google.com','key27','value27');
INSERT INTO "values_mapping" VALUES ('http://google.com','key28','value28');
INSERT INTO "values_mapping" VALUES ('http://google.com','key29','value29');
INSERT INTO "values_mapping" VALUES ('http://google.com','key30','value30');
INSERT INTO "values_mapping" VALUES ('http://google.com','key31','value31');
INSERT INTO "values_mapping" VALUES ('http://google.com','key32','value32');
INSERT INTO "values_mapping" VALUES ('http://google.com','key33','value33');
INSERT INTO "values_mapping" VALUES ('http://google.com','key34','value34');
INSERT INTO "values_mapping" VALUES ('http://google.com','key35','value35');
INSERT INTO "values_mapping" VALUES ('http://google.com','key36','value36');
INSERT INTO "values_mapping" VALUES ('http://google.com','key37','value37');
INSERT INTO "values_mapping" VALUES ('http://google.com','key38','value38');
INSERT INTO "values_mapping" VALUES ('http://google.com','key39','value39');
INSERT INTO "values_mapping" VALUES ('http://google.com','key40','value40');
INSERT INTO "values_mapping" VALUES ('http://google.com','key41','value41');
INSERT INTO "values_mapping" VALUES ('http://google.com','key42','value42');
INSERT INTO "values_mapping" VALUES ('http://google.com','key43','value43');
INSERT INTO "values_mapping" VALUES ('http://google.com','key44','value44');
INSERT INTO "values_mapping" VALUES ('http://google.com','key45','value45');
INSERT INTO "values_mapping" VALUES ('http://google.com','key46','value46');
INSERT INTO "values_mapping" VALUES ('http://google.com','key47','value47');
INSERT INTO "values_mapping" VALUES ('http://google.com','key48','value48');
INSERT INTO "values_mapping" VALUES ('http://google.com','key49','value49');
INSERT INTO "values_mapping" VALUES ('http://google.com','key50','value50');
INSERT INTO "values_mapping" VALUES ('http://google.com','key51','value51');
INSERT INTO "values_mapping" VALUES ('http://google.com','key52','value52');
INSERT INTO "values_mapping" VALUES ('http://google.com','key53','value53');
INSERT INTO "values_mapping" VALUES ('http://google.com','key54','value54');
INSERT INTO "values_mapping" VALUES ('http://google.com','key55','value55');
INSERT INTO "values_mapping" VALUES ('http://google.com','key56','value56');
INSERT INTO "values_mapping" VALUES ('http://google.com','key57','value57');
INSERT INTO "values_mapping" VALUES ('http://google.com','key58','value58');
INSERT INTO "values_mapping" VALUES ('http://google.com','key59','value59');
INSERT INTO "values_mapping" VALUES ('http://google.com','key60','value60');
INSERT INTO "values_mapping" VALUES ('http://google.com','key61','value61');
INSERT INTO "values_mapping" VALUES ('http://google.com','key62','value62');
INSERT INTO "values_mapping" VALUES ('http://google.com','key63','value63');
INSERT INTO "values_mapping" VALUES ('http://google.com','key64','value64');
INSERT INTO "values_mapping" VALUES ('http://google.com','key65','value65');
INSERT INTO "values_mapping" VALUES ('http://google.com','key66','value66');
INSERT INTO "values_mapping" VALUES ('http://google.com','key67','value67');
INSERT INTO "values_mapping" VALUES ('http://google.com','key68','value68');
INSERT INTO "values_mapping" VALUES ('http://google.com','key69','value69');
INSERT INTO "values_mapping" VALUES ('http://google.com','key70','value70');
INSERT INTO "values_mapping" VALUES ('http://google.com','key71','value71');
INSERT INTO "values_mapping" VALUES ('http://google.com','key72','value72');
INSERT INTO "values_mapping" VALUES ('http://google.com','key73','value73');
INSERT INTO "values_mapping" VALUES ('http://google.com','key74','value74');
INSERT INTO "values_mapping" VALUES ('http://google.com','key75','value75');
INSERT INTO "values_mapping" VALUES ('http://google.com','key76','value76');
INSERT INTO "values_mapping" VALUES ('http://google.com','key77','value77');
INSERT INTO "values_mapping" VALUES ('http://google.com','key78','value78');
INSERT INTO "values_mapping" VALUES ('http://google.com','key79','value79');
INSERT INTO "values_mapping" VALUES ('http://google.com','key80','value80');
INSERT INTO "values_mapping" VALUES ('http://google.com','key81','value81');
INSERT INTO "values_mapping" VALUES ('http://google.com','key82','value82');
INSERT INTO "values_mapping" VALUES ('http://google.com','key83','value83');
INSERT INTO "values_mapping" VALUES ('http://google.com','key84','value84');
INSERT INTO "values_mapping" VALUES ('http://google.com','key85','value85');
INSERT INTO "values_mapping" VALUES ('http://google.com','key86','value86');
INSERT INTO "values_mapping" VALUES ('http://google.com','key87','value87');
INSERT INTO "values_mapping" VALUES ('http://google.com','key88','value88');
INSERT INTO "values_mapping" VALUES ('http://google.com','key89','value89');
INSERT INTO "values_mapping" VALUES ('http://google.com','key90','value90');
INSERT INTO "values_mapping" VALUES ('http://google.com','key91','value91');
INSERT INTO "values_mapping" VALUES ('http://google.com','key92','value92');
INSERT INTO "values_mapping" VALUES ('http://google.com','key93','value93');
INSERT INTO "values_mapping" VALUES ('http://google.com','key94','value94');
INSERT INTO "values_mapping" VALUES ('http://google.com','key95','value95');
INSERT INTO "values_mapping" VALUES ('http://google.com','key96','value96');
INSERT INTO "values_mapping" VALUES ('http://google.com','key97','value97');
INSERT INTO "values_mapping" VALUES ('http://google.com','key98','value98');
INSERT INTO "values_mapping" VALUES ('http://google.com','key99','value99');
INSERT INTO "values_mapping" VALUES ('http://google.com','key100','value100');
INSERT INTO "values_mapping" VALUES ('http://google.com','key101','value101');
INSERT INTO "values_mapping" VALUES ('http://google.com','key102','value102');
INSERT INTO "values_mapping" VALUES ('http://google.com','key103','value103');
INSERT INTO "values_mapping" VALUES ('http://google.com','key104','value104');
INSERT INTO "values_mapping" VALUES ('http://google.com','key105','value105');
INSERT INTO "values_mapping" VALUES ('http://google.com','key106','value106');
INSERT INTO "values_mapping" VALUES ('http://google.com','key107','value107');
INSERT INTO "values_mapping" VALUES ('http://google.com','key108','value108');
INSERT INTO "values_mapping" VALUES ('http://google.com','key109','value109');
INSERT INTO "values_mapping" VALUES ('http://google.com','key110','value110');
INSERT INTO "values_mapping" VALUES ('http://google.com','key111','value111');
INSERT INTO "values_mapping" VALUES ('http://google.com','key112','value112');
INSERT INTO "values_mapping" VALUES ('http://google.com','key113','value113');
INSERT INTO "values_mapping" VALUES ('http://google.com','key114','value114');
INSERT INTO "values_mapping" VALUES ('http://google.com','key115','value115');
INSERT INTO "values_mapping" VALUES ('http://google.com','key116','value116');
INSERT INTO "values_mapping" VALUES ('http://google.com','key117','value117');
INSERT INTO "values_mapping" VALUES ('http://google.com','key118','value118');
INSERT INTO "values_mapping" VALUES ('http://google.com','key119','value119');
INSERT INTO "values_mapping" VALUES ('http://google.com','key120','value120');
INSERT INTO "values_mapping" VALUES ('http://google.com','key121','value121');
INSERT INTO "values_mapping" VALUES ('http://google.com','key122','value122');
INSERT INTO "values_mapping" VALUES ('http://google.com','key123','value123');
INSERT INTO "values_mapping" VALUES ('http://google.com','key124','value124');
INSERT INTO "values_mapping" VALUES ('http://google.com','key125','value125');
INSERT INTO "values_mapping" VALUES ('http://google.com','key126','value126');
INSERT INTO "values_mapping" VALUES ('http://google.com','key127','value127');
INSERT INTO "values_mapping" VALUES ('http://google.com','key128','value128');
INSERT INTO "values_mapping" VALUES ('http://google.com','key129','value129');
INSERT INTO "values_mapping" VALUES ('http://google.com','key130','value130');
INSERT INTO "values_mapping" VALUES ('http://google.com','key131','value131');
INSERT INTO "values_mapping" VALUES ('http://google.com','key132','value132');
INSERT INTO "values_mapping" VALUES ('http://google.com','key133','value133');
INSERT INTO "values_mapping" VALUES ('http://google.com','key134','value134');
INSERT INTO "values_mapping" VALUES ('http://google.com','key135','value135');
INSERT INTO "values_mapping" VALUES ('http://google.com','key136','value136');
INSERT INTO "values_mapping" VALUES ('http://google.com','key137','value137');
INSERT INTO "values_mapping" VALUES ('http://google.com','key138','value138');
INSERT INTO "values_mapping" VALUES ('http://google.com','key139','value139');
INSERT INTO "values_mapping" VALUES ('http://google.com','key140','value140');
INSERT INTO "values_mapping" VALUES ('http://google.com','key141','value141');
INSERT INTO "values_mapping" VALUES ('http://google.com','key142','value142');
INSERT INTO "values_mapping" VALUES ('http://google.com','key143','value143');
INSERT INTO "values_mapping" VALUES ('http://google.com','key144','value144');
INSERT INTO "values_mapping" VALUES ('http://google.com','key145','value145');
INSERT INTO "values_mapping" VALUES ('http://google.com','key146','value146');
INSERT INTO "values_mapping" VALUES ('http://google.com','key147','value147');
INSERT INTO "values_mapping" VALUES ('http://google.com','key148','value148');
INSERT INTO "values_mapping" VALUES ('http://google.com','key149','value149');
INSERT INTO "values_mapping" VALUES ('http://google.com','key150','value150');
INSERT INTO "values_mapping" VALUES ('http://google.com','key151','value151');
INSERT INTO "values_mapping" VALUES ('http://google.com','key152','value152');
INSERT INTO "values_mapping" VALUES ('http://google.com','key153','value153');
INSERT INTO "values_mapping" VALUES ('http://google.com','key154','value154');
INSERT INTO "values_mapping" VALUES ('http://google.com','key155','value155');
INSERT INTO "values_mapping" VALUES ('http://google.com','key156','value156');
INSERT INTO "values_mapping" VALUES ('http://google.com','key157','value157');
INSERT INTO "values_mapping" VALUES ('http://google.com','key158','value158');
INSERT INTO "values_mapping" VALUES ('http://google.com','key159','value159');
INSERT INTO "values_mapping" VALUES ('http://google.com','key160','value160');
INSERT INTO "values_mapping" VALUES ('http://google.com','key161','value161');
INSERT INTO "values_mapping" VALUES ('http://google.com','key162','value162');
INSERT INTO "values_mapping" VALUES ('http://google.com','key163','value163');
INSERT INTO "values_mapping" VALUES ('http://google.com','key164','value164');
INSERT INTO "values_mapping" VALUES ('http://google.com','key165','value165');
INSERT INTO "values_mapping" VALUES ('http://google.com','key166','value166');
INSERT INTO "values_mapping" VALUES ('http://google.com','key167','value167');
INSERT INTO "values_mapping" VALUES ('http://google.com','key168','value168');
INSERT INTO "values_mapping" VALUES ('http://google.com','key169','value169');
INSERT INTO "values_mapping" VALUES ('http://google.com','key170','value170');
INSERT INTO "values_mapping" VALUES ('http://google.com','key171','value171');
INSERT INTO "values_mapping" VALUES ('http://google.com','key172','value172');
INSERT INTO "values_mapping" VALUES ('http://google.com','key173','value173');
INSERT INTO "values_mapping" VALUES ('http://google.com','key174','value174');
INSERT INTO "values_mapping" VALUES ('http://google.com','key175','value175');
INSERT INTO "values_mapping" VALUES ('http://google.com','key176','value176');
INSERT INTO "values_mapping" VALUES ('http://google.com','key177','value177');
INSERT INTO "values_mapping" VALUES ('http://google.com','key178','value178');
INSERT INTO "values_mapping" VALUES ('http://google.com','key179','value179');
INSERT INTO "values_mapping" VALUES ('http://google.com','key180','value180');
INSERT INTO "values_mapping" VALUES ('http://google.com','key181','value181');
INSERT INTO "values_mapping" VALUES ('http://google.com','key182','value182');
INSERT INTO "values_mapping" VALUES ('http://google.com','key183','value183');
INSERT INTO "values_mapping" VALUES ('http://google.com','key184','value184');
INSERT INTO "values_mapping" VALUES ('http://google.com','key185','value185');
INSERT INTO "values_mapping" VALUES ('http://google.com','key186','value186');
INSERT INTO "values_mapping" VALUES ('http://google.com','key187','value187');
INSERT INTO "values_mapping" VALUES ('http://google.com','key188','value188');
INSERT INTO "values_mapping" VALUES ('http://google.com','key189','value189');
INSERT INTO "values_mapping" VALUES ('http://google.com','key190','value190');
INSERT INTO "values_mapping" VALUES ('http://google.com','key191','value191');
INSERT INTO "values_mapping" VALUES ('http://google.com','key192','value192');
INSERT INTO "values_mapping" VALUES ('http://google.com','key193','value193');
INSERT INTO "values_mapping" VALUES ('http://google.com','key194','value194');
INSERT INTO "values_mapping" VALUES ('http://google.com','key195','value195');
INSERT INTO "values_mapping" VALUES ('http://google.com','key196','value196');
INSERT INTO "values_mapping" VALUES ('http://google.com','key197','value197');
INSERT INTO "values_mapping" VALUES ('http://google.com','key198','value198');
INSERT INTO "values_mapping" VALUES ('http://google.com','key199','value199');
INSERT INTO "values_mapping" VALUES ('http://google.com','key200','value200');
INSERT INTO "values_mapping" VALUES ('http://google.com','key201','value201');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','A','valueA');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','B','valueB');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','C','valueC');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','D','valueD');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','E','valueE');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','F','valueF');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','G','valueG');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','H','valueH');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','I','valueI');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','J','valueJ');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','K','valueK');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','L','valueL');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','M','valueM');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','N','valueN');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','O','valueO');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','P','valueP');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','Q','valueQ');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','R','valueR');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','S','valueS');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','T','valueT');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','U','valueU');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','V','valueV');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','W','valueW');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','X','valueX');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','Y','valueY');
INSERT INTO "values_mapping" VALUES ('http://chromium.org','Z','valueZ');
CREATE TABLE per_origin_mapping(context_origin TEXT NOT NULL PRIMARY KEY,last_used_time INTEGER NOT NULL,length INTEGER NOT NULL) WITHOUT ROWID;
INSERT INTO "per_origin_mapping" VALUES ('http://google.com',13266954476192362,201);
INSERT INTO "per_origin_mapping" VALUES ('http://chromium.org',13268941676192362,26);
CREATE TABLE budget_mapping(id INTEGER NOT NULL PRIMARY KEY,context_origin TEXT NOT NULL,time_stamp INTEGER NOT NULL,bits_debit REAL NOT NULL);
CREATE INDEX IF NOT EXISTS per_origin_mapping_last_used_time_idx ON per_origin_mapping(last_used_time);
CREATE INDEX IF NOT EXISTS budget_mapping_origin_time_stamp_idx ON budget_mapping(context_origin,time_stamp);
COMMIT;
