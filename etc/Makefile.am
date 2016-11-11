## Makefile.am -- Process this file with automake to produce Makefile.in



dist_sysconf_DATA = netqos_conf.dtd filterdef.dtd filterdef.xml \
                    filterval.dtd filterval.xml \
                    reply.xml reply.xsl reply2.xsl \
		    example_rules1.xml example_rules2.xml example_rules3.xml \
		    main.html 
		    

nodist_sysconf_DATA = netqos_conf.xml


DISTCLEANFILES = gen_cert.sh netqos_conf.xml



if ENABLE_SSL

  nodist_bin_SCRIPTS = gen_cert.sh 

  DISTCLEANFILES += netqos.pem

endif



install-exec-hook: 

if ENABLE_SSL

	@echo "Generating certificate for SSL" ; \
	$(DESTDIR)/$(bindir)/gen_cert.sh $(DESTDIR)/$(sysconfdir)

endif