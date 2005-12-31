/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://monetdb.cwi.nl/Legal/MonetDBLicense-1.1.html
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-2005 CWI.
 * All Rights Reserved.
 */

package nl.cwi.monetdb.client;

import nl.cwi.monetdb.client.util.*;
import java.sql.*;
import java.io.*;
import java.util.*;
import java.util.zip.*;
import java.net.*;

/**
 * This program acts like an extended client program for MonetDB. Its
 * look and feel is very much like PostgreSQL's interactive terminal
 * program.  Although it looks like this client is designed for MonetDB,
 * it demonstrates the power of the JDBC interface since it built on top
 * of JDBC only.
 *
 * @author Fabian Groffen <Fabian.Groffen@cwi.nl>
 * @version 1.1
 */

public class JdbcClient {
	private static Connection con;
	private static Statement stmt;
	private static BufferedReader in;
	private static PrintWriter out;
	private static Exporter e;
	private static DatabaseMetaData dbmd;

	public final static void main(String[] args) throws Exception {
		CmdLineOpts copts = new CmdLineOpts();

		// arguments which need exactly one value
		copts.addOption("h", "host", CmdLineOpts.CAR_ONE, "localhost");
		copts.addOption("p", "port", CmdLineOpts.CAR_ONE, "45123");
		copts.addOption("u", "user", CmdLineOpts.CAR_ONE, System.getProperty("user.name"));
		copts.addOption("b", "database", CmdLineOpts.CAR_ONE, "demo");
		copts.addOption("l", "language", CmdLineOpts.CAR_ONE, "sql");
		copts.addOption(null, "Xblksize", CmdLineOpts.CAR_ONE, null);
		copts.addOption(null, "Xoutput", CmdLineOpts.CAR_ONE, null);
		copts.addOption(null, "Xprepare", CmdLineOpts.CAR_ONE, "native");
		// todo make it CAR_ONE_MANY
		copts.addOption("f", "file", CmdLineOpts.CAR_ONE, null);
		// this one is only here for the .monetdb file parsing, it is
		// removed before the command line arguments are parsed
		copts.addOption(null, "password", CmdLineOpts.CAR_ONE, null);

		// arguments which can have zero to lots of arguments
		copts.addOption("d", "dump", CmdLineOpts.CAR_ZERO_MANY, null);

		// arguments which can have zero or one argument(s)
		copts.addOption(null, "Xdebug", CmdLineOpts.CAR_ZERO_ONE, null);
		copts.addOption(null, "Xbatching", CmdLineOpts.CAR_ZERO_ONE, null);

		// arguments which have no argument(s)
		copts.addOption(null, "help", CmdLineOpts.CAR_ZERO, null);
		copts.addOption("e", "echo", CmdLineOpts.CAR_ZERO, null);

		// we store user and password in separate variables in order to
		// be able to properly act on them like forgetting the password
		// from the user's file if the user supplies a username on the
		// command line arguments
		String pass = null;
		String user = null;

		// look for a file called .monetdb in the current dir or in the
		// user's homedir and read its preferences
		File pref = new File(".monetdb");
		if (!pref.exists()) pref = new File(System.getProperty("user.home"), ".monetdb");
		if (pref.exists()) {
			try {
				copts.processFile(pref);
			} catch (OptionsException e) {
				System.err.println("Error in " + pref.getAbsolutePath() + ": " + e.getMessage());
				System.exit(-1);
			}
			user = copts.getOption("user").getArgument();
			pass = copts.getOption("password").getArgument();
		}

		// process the command line arguments, remove password option
		// first, and save the user we had at this point
		copts.removeOption("password");
		try {
			copts.processArgs(args);
		} catch (OptionsException e) {
			System.err.println("Error: " + e.getMessage());
			System.exit(-1);
		}
		// we can actually compare pointers (objects) here
		if (user != copts.getOption("user").getArgument()) pass = null;

		if (copts.getOption("help").isPresent()) {
			System.out.print(
"Usage java -jar MonetDB_JDBC.jar [-h host[:port]] [-p port] [-f file] [-u user]\n" +
"                                 [-l language] [-b [database]] [-d [table]]\n" +
"                                 [-e] [-X<opt>]\n" +
"or using long option equivalents --host --port --file --user --language\n" +
"--dump --echo --database.\n" +
"Arguments may be written directly after the option like -p45123.\n" +
"\n" +
"If no host and port are given, localhost and 45123 are assumed.  An .monetdb\n" +
"file may exist in the user's home directory.  This file can contain\n" +
"preferences to use each time JdbcClient is started.  Options given on the\n" +
"command line override the preferences file.  The .monetdb file syntax is\n" +
"<option>=<value> where option is one of the options host, port, file, mode\n" +
"debug, or password.  Note that the last one is perilous and therefore not\n" +
"available as command line option.\n" +
"If no input file is given using the -f flag, an interactive session is\n" +
"started on the terminal.\n" +
"\n" +
"OPTIONS\n" +
"-h --host  The hostname of the host that runs the MonetDB database.  A port\n" +
"           number can be supplied by use of a colon, i.e. -h somehost:12345.\n" +
"-p --port  The port number to connect to.\n" +
"-f --file  A file name to use either for reading or writing.  The file will\n" +
"           be used for writing when dump mode is used (-d --dump).\n" +
"           In read mode, the file can also be an URL pointing to a plain\n" +
"           text file that is optionally gzip compressed.\n" +
"-u --user  The username to use when connecting to the database.\n" +
"-l --language  Use the given language, for example 'xquery'.\n" +
"-d --dump  Dumps the given table(s), or the complete database if none given.\n" +
"--help     This screen.\n" +
"-e --echo  Also outputs the contents of the input file, if any.\n" +
"-b --database  Try to connect to the given database (only makes sense\n" +
"           if connecting to a DatabasePool or equivalent process).\n" +
"\n" +
"EXTRA OPTIONS\n" +
"-Xdebug    Writes a transmission log to disk for debugging purposes.  If a\n" +
"           file name is given, it is used, otherwise a file called\n" +
"           monet<timestamp>.log is created.  A given file will never be\n" +
"           overwritten; instead a unique variation of the file is used.\n" +
"-Xblksize  Specifies the blocksize when using block mode, given in bytes.\n" +
"-Xoutput   The output mode when dumping.  Default is sql, xml may be used for\n" +
"           an experimental XML output.\n" +
"-Xbatching Indicates that a batch should be used instead of direct\n" +
"           communication with the server for each statement.  If a number is\n" +
"           given, it is used as batch size.  I.e. 8000 would execute the\n" +
"           contents on the batch after each 8000 read rows.  Batching can\n" +
"           greatly speedup the process of restoring a database dump.\n" +
/*
"-Xprepare  Specifies which PreparedStatement to use.  Valid arguments are:\n" +
"           'native' and 'java'.  The default behaviour if not specified or\n" +
"           with an unknown value is to request for a native PreparedStatement\n" +
"           at the MonetDB JDBC driver.\n" +
*/
""
);
			System.exit(0);
		}

		in = new BufferedReader(new InputStreamReader(System.in));
		out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(System.out)));

		// we need the password from the user, fetch it with a pseudo
		// password protector
		if (pass == null) {
			try {
				char[] tmp = PasswordField.getPassword(System.in, "password: ");
				if (tmp != null) pass = String.valueOf(tmp);
			} catch (IOException ioe) {
				System.err.println("Invalid password!");
				System.exit(-1);
			}
			System.out.println("");
		}

		user = copts.getOption("user").getArgument();

		// build the hostname
		String host = copts.getOption("host").getArgument();
		if (host.indexOf(":") == -1) {
			host = host + ":" + copts.getOption("port").getArgument();
		}

		// make sure the driver is loaded
		Class.forName("nl.cwi.monetdb.jdbc.MonetDriver");

		// build the extra arguments of the JDBC connect string
		String attr = "?";
		CmdLineOpts.OptionContainer oc = copts.getOption("Xblksize");
		if (oc.isPresent())
			attr += "blockmode_blocksize=" + oc.getArgument() + "&";
		oc = copts.getOption("Xprepare");
		if (oc.isPresent())
			attr += "native_prepared_statements=" + ("java".equals(oc.getArgument()) ? "false" : "true") + "&";
		oc = copts.getOption("language");
		String lang = oc.getArgument();
		if (oc.isPresent())
			attr += "language=" + lang + "&";
		oc = copts.getOption("Xdebug");
		if (oc.isPresent()) {
			attr += "debug=true&";
			if (oc.getArgumentCount() == 1)
				attr += "logfile=" + oc.getArgument() + "&";
		}

		// request a connection suitable for MonetDB from the driver
		// manager note that the database specifier is only used when
		// connecting to a proxy-like service, since MonetDB itself
		// can't access multiple databases.
		con = null;
		String database = copts.getOption("database").getArgument();
		try {
			con = DriverManager.getConnection(
					"jdbc:monetdb://" + host + "/" + database + attr,
					user,
					pass
			);
			SQLWarning warn = con.getWarnings();
			while (warn != null) {
				System.err.println("Connection warning: " +
					warn.getMessage());
				warn = warn.getNextWarning();
			}
			con.clearWarnings();
		} catch (SQLException e) {
			System.err.println("Database connect failed: " + e.getMessage());
			System.exit(-1);
		}
		try {
			dbmd = con.getMetaData();
		} catch (SQLException e) {
			// we ignore this because it's probably because we don't use
			// SQL language
			dbmd = null;
		}
		stmt = con.createStatement();
		
		boolean xmlMode =
				"xml".equals(copts.getOption("Xoutput").getArgument());

		// see if we will have to perform a database dump (only in SQL
		// mode)
		if ("sql".equals(lang) && copts.getOption("dump").isPresent()) {
			ResultSet tbl;

			// use the given file for writing
			oc = copts.getOption("file");
			if (oc.isPresent())
				out = new PrintWriter(new BufferedWriter(new FileWriter(oc.getArgument())));

			// we only want tables and views to be dumped, unless a specific
			// table is requested
			String[] types = {"TABLE", "VIEW"};
			if (copts.getOption("dump").getArgumentCount() > 0) types = null;
			// request the tables available in the database
			tbl = dbmd.getTables(null, null, null, types);

			LinkedList tables = new LinkedList();
			while (tbl.next()) {
				tables.add(new Table(
					tbl.getString("TABLE_CAT"),
					tbl.getString("TABLE_SCHEM"),
					tbl.getString("TABLE_NAME"),
					tbl.getString("TABLE_TYPE")));
			}

			if (xmlMode) {
				e = new XMLExporter(out);
				e.setProperty(XMLExporter.TYPE_NIL, XMLExporter.VALUE_XSI);
			} else {
				e = new SQLExporter(out);
				// stick with inserts for now, in the future we might do
				// COPY INTO's here using VALUE_COPY
				e.setProperty(SQLExporter.TYPE_OUTPUT, SQLExporter.VALUE_INSERT);
			}
			e.useSchemas(true);

			// start SQL output
			if (!xmlMode) out.println("START TRANSACTION;\n");

			// dump specific table(s) or not?
			if (copts.getOption("dump").getArgumentCount() > 0) { // yes we do
				String[] dumpers = copts.getOption("dump").getArguments();
				for (int i = 0; i < tables.size(); i++) {
					Table ttmp = (Table)(tables.get(i));
					for (int j = 0; j < dumpers.length; j++) {
						if (ttmp.getName().equalsIgnoreCase(dumpers[j].toString()) ||
							ttmp.getFqname().equalsIgnoreCase(dumpers[j].toString()))
						{
							// dump the table
							doDump(out, ttmp, dbmd, stmt);
						}
					}
				}
			} else {
				tbl = dbmd.getImportedKeys(null, null, null);
				while (tbl.next()) {
					// find FK table object
					Table fk = Table.findTable(tbl.getString("FKTABLE_SCHEM") + "." + tbl.getString("FKTABLE_NAME"), tables);

					// find PK table object
					Table pk = Table.findTable(tbl.getString("PKTABLE_SCHEM") + "." + tbl.getString("PKTABLE_NAME"), tables);

					// should not be possible to happen
					if (fk == null || pk == null)
						throw new AssertionError("Illegal table; table not found in list");

					// add PK table dependancy to FK table
					fk.addDependancy(pk);
				}

				// search for cycles of type a -> (x ->)+ b probably not
				// the most optimal way, but it works by just scanning
				// every table for loops in a recursive manor
				for (int i = 0; i < tables.size(); i++) {
					Table.checkForLoop((Table)(tables.get(i)), new ArrayList());
				}

				// find the graph, at this point we know there are no
				// cycles, thus a solution exists
				for (int i = 0; i < tables.size(); i++) {
					List needs = ((Table)(tables.get(i))).requires(tables.subList(0, i + 1));
					if (needs.size() > 0) {
						tables.removeAll(needs);
						tables.addAll(i, needs);

						// re-evaluate this position, for there is a new
						// table now
						i--;
					}
				}

				// we now have the right order to dump tables
				for (int i = 0; i < tables.size(); i++) {
					// dump the table
					Table t = (Table)(tables.get(i));
					doDump(out, t, dbmd, stmt);
				}
			}

			if (!xmlMode) out.println("COMMIT;");
			out.flush();

			con.close();
			System.exit(0);
		}

		if (xmlMode) {
			e = new XMLExporter(out);
			e.setProperty(XMLExporter.TYPE_NIL, XMLExporter.VALUE_XSI);
		} else {
			e = new SQLExporter(out);
			// we want nice table views
			e.setProperty(SQLExporter.TYPE_OUTPUT, SQLExporter.VALUE_TABLE);
		}
		e.useSchemas(false);

		try {
			// use the given file for reading
			boolean hasFile = copts.getOption("file").isPresent();
			boolean doEcho = hasFile && copts.getOption("echo").isPresent();
			if (hasFile) {
				String tmp = copts.getOption("file").getArgument();
				int batchSize = 0;
				// figure out whether it is an URL
				try {
					URL u = new URL(tmp);
					HttpURLConnection.setFollowRedirects(true);
					HttpURLConnection con =
						(HttpURLConnection)u.openConnection();
					con.setRequestMethod("GET");
					String ct = con.getContentType();
					if ("application/x-gzip".equals(ct)) {
						// open gzip stream
						in = new BufferedReader(new InputStreamReader(
							new GZIPInputStream(con.getInputStream())));
					} else {
						// text/plain otherwise just attempt to read as is
						in = new BufferedReader(new InputStreamReader(
							con.getInputStream()));
					}
				} catch (MalformedURLException e) {
					// probably a real file, open it
					in = new BufferedReader(new FileReader(tmp));
				}

				// check for batch mode
				oc = copts.getOption("Xbatching");
				if (oc.isPresent()) {
					if (oc.getArgumentCount() == 1) {
						// parse the number
						try {
							batchSize = Integer.parseInt(oc.getArgument());
						} catch (NumberFormatException ex) {
							// complain to the user
							throw new IllegalArgumentException("Illegal argument for Xbatching: " + oc.getArgument() + " is not a parseable number!");
						}
					}
					processBatch(batchSize);
				} else {
					processInteractive(true, doEcho, user);
				}
			} else {
				// print welcome message
				out.println("Welcome to the MonetDB interactive JDBC terminal!");
				if (dbmd != null) {
					out.println("Database: " + dbmd.getDatabaseProductName() + " " +
						dbmd.getDatabaseProductVersion());
					out.println("Driver: " + dbmd.getDriverName() + " " +
						dbmd.getDriverVersion());
				}
				out.println("Type \\q to quit, \\h for a list of available commands");
				out.flush();
				processInteractive(false, doEcho, user);
			}

			// free resources, close the statement
			stmt.close();
			// close the connection with the database
			con.close();
			// close the file (if we used a file)
			in.close();
		} catch (Exception e) {
			System.err.println("A fatal exception occurred: " + e.toString());
			e.printStackTrace(System.err);
			// at least try to close the connection properly, since it will
			// close all statements associated with it
			try {
				con.close();
			} catch (SQLException ex) {
				// ok... nice try
			}
			System.exit(-1);
		}
	}

	/**
	 * Starts an interactive processing loop, where output is adjusted to an
	 * user session.  This processing loop is not suitable for bulk processing
	 * as in executing the contents of a file, since processing on the given
	 * input is done after each row that has been entered.
	 *
	 * @param hasFile a boolean indicating whether a file is used as input
	 * @param doEcho a boolean indicating whether to echo the given input
	 * @param user a String representing the username of the current user
	 * @throws IOException if an IO exception occurs
	 * @throws SQLException if a database related error occurs
	 */
	public static void processInteractive(
		boolean hasFile,
		boolean doEcho,
		String user
	)
		throws IOException, SQLException
	{
		// an SQL stack keeps track of ( " and '
		SQLStack stack = new SQLStack();
		// a query part is a line of an SQL query
		QueryPart qp = null;

		String query = "", curLine;
		boolean wasComplete = true, doProcess, lastac = false;
		if (!hasFile) {
			lastac = con.getAutoCommit();
			out.println("auto commit mode: " + (lastac ? "on" : "off"));
			out.print(getPrompt(user, stack, true));
			out.flush();
		}

		// the main (interactive) process loop
		int i = 0;
		for (i = 1; (curLine = in.readLine()) != null; i++) {
			if (doEcho) {
				out.println(curLine);
				out.flush();
			}
			qp = scanQuery(curLine, stack);
			if (!qp.isEmpty()) {
				doProcess = true;
				if (wasComplete) {
					doProcess = false;
					// check for commands only when the previous row was
					// complete
					if (qp.getQuery().equals("\\q")) {
						// quit
						break;
					} else if (qp.getQuery().startsWith("\\h")) {
						out.println("Available commands:");
						out.println("\\q      quits this program");
						out.println("\\h      this help screen");
						out.println("\\d      list available tables and views");
						out.println("\\d<obj> describes the given table or view");
					} else if (dbmd != null && qp.getQuery().startsWith("\\d")) {
						String object = qp.getQuery().substring(2).trim().toLowerCase();
						if (object.endsWith(";")) object = object.substring(0, object.length() - 1);
						if (!object.equals("")) {
							ResultSet tbl = dbmd.getTables(null, null, null, null);
							// we have an object, see is we can find it
							boolean found = false;
							while (tbl.next()) {
								if (tbl.getString("TABLE_NAME").equalsIgnoreCase(object) ||
									(tbl.getString("TABLE_SCHEM") + "." + tbl.getString("TABLE_NAME")).equalsIgnoreCase(object)) {
									// we found it, describe it
									e.dumpSchema(
										dbmd,
										tbl.getString("TABLE_TYPE"),
										tbl.getString("TABLE_CAT"),
										tbl.getString("TABLE_SCHEM"),
										tbl.getString("TABLE_NAME")
									);
									
									found = true;
									break;
								}
							}
							if (!found) System.err.println("Unknown table or view: " + object);
							tbl.close();
						} else {
							String[] types = {"TABLE", "VIEW"};
							ResultSet tbl = dbmd.getTables(null, null, null, types);
							// give us a list with tables
							while (tbl.next()) {
								out.println(tbl.getString("TABLE_TYPE") + "\t" +
									tbl.getString("TABLE_SCHEM") + "." +
									tbl.getString("TABLE_NAME"));
							}
							tbl.close();
						}
					} else {
						doProcess = true;
					}
				}

				if (doProcess) {
					query += qp.getQuery() + (qp.hasOpenQuote() ? "\\n" : " ");
					if (qp.isComplete()) {
						// strip off trailing ';'
						query = query.substring(0, query.length() - 2);
						// execute query
						try {
							executeQuery(query, stmt, out);
						} catch (SQLException e) {
							if (hasFile) {
								System.err.println("Error on line " + i + ": " + e.getMessage());
							} else {
								System.err.println("Error: " + e.getMessage());
							}
						}
						query = "";
					}
					wasComplete = qp.isComplete();
				}
			}
			if (!hasFile) {
				boolean ac = con.getAutoCommit();
				if (ac != lastac) {
					out.println("auto commit mode: " + (ac ? "on" : "off"));
					lastac = ac;
				}
				out.print(getPrompt(user, stack, wasComplete));
			}
			out.flush();
		}
		if (qp != null) {
			try {
				if (query != "") executeQuery(query, stmt, out);
			} catch (SQLException e) {
				if (hasFile) {
					System.err.println("Error on line " + i + ": " + e.getMessage());
				} else {
					System.err.println("Error: " + e.getMessage());
				}
			}
		}
	}

	/**
	 * Executes the given query and prints the result tabularised to the
	 * given PrintWriter stream.  The result of this method is the
	 * default output of a query: tabular data.
	 *
	 * @param query the query to execute
	 * @param stmt the Statement to execute the query on
	 * @param out the PrintWriter to write to
	 * @throws SQLException if a database related error occurs
	 */
	private static void executeQuery(String query,
			Statement stmt,
			PrintWriter out)
		throws SQLException
	{
		// warnings generated during querying
		SQLWarning warn;

		// execute the query, let the driver decide what type it is
		int aff = -1;
		boolean	nextRslt = stmt.execute(query);
		if (!nextRslt) aff = stmt.getUpdateCount();
		do {
			if (nextRslt) {
				// we have a ResultSet, print it
				ResultSet rs = stmt.getResultSet();

				e.dumpResultSet(rs);

				// if there were warnings for this result,
				// show them!
				warn = rs.getWarnings();
				if (warn != null) {
					// force stdout to be written so the
					// warning appears below it
					out.flush();
					do {
						System.err.println("ResultSet warning: " +
							warn.getMessage());
						warn = warn.getNextWarning();
					} while (warn != null);
					rs.clearWarnings();
				}
				rs.close();
			} else if (aff != -1) {
				if (aff == Statement.SUCCESS_NO_INFO) {
					out.println("Operation successful");
				} else {
					// we have an update count
					out.println(aff + " affected row" + (aff != 1 ? "s" : ""));
				}
			}

			out.println();
			out.flush();
		} while ((nextRslt = stmt.getMoreResults()) ||
				 (aff = stmt.getUpdateCount()) != -1);

		// if there were warnings for this statement,
		// and/or connection show them!
		warn = stmt.getWarnings();
		while (warn != null) {
			System.err.println("Statement warning: " +
				warn.getMessage());
			warn = warn.getNextWarning();
		}
		stmt.clearWarnings();
		warn = con.getWarnings();
		while (warn != null) {
			System.err.println("Connection warning: " +
				warn.getMessage());
			warn = warn.getNextWarning();
		}
		con.clearWarnings();
	}

	/**
	 * Starts a processing loop optimized for processing (large) chunks of
	 * continous data, such as input from a file.  Unlike in the interactive
	 * loop above, queries are sent only to the database if a certain batch
	 * amount is reached.  No client side query checks are made, but everything
	 * is sent to the server as-is.
	 *
	 * @param batchSize the number of items to store in the batch before
	 *                  sending them to the database for execution.
	 * @throws IOException if an IO exception occurs.
	 */
	public static void processBatch(int batchSize) throws IOException {
		StringBuffer query = new StringBuffer();
		String curLine;
		int i = 0;
		try {
			// because this is an explicit batch from a file, we turn off
			// auto-commit
			con.setAutoCommit(false);

			// the main loop
			for (i = 1; (curLine = in.readLine()) != null; i++) {
				query.append(curLine);
				if (curLine.endsWith(";")) {
					// lousy check for end of statement, but in batch mode it
					// is not very important to catch all end of statements...
					stmt.addBatch(query.toString());
					query.delete(0, query.length());
				} else {
					query.append('\n');
				}
				if (batchSize > 0 && i % batchSize == 0) {
					stmt.executeBatch();
					stmt.clearBatch();
				}
			}
			stmt.addBatch(query.toString());
			stmt.executeBatch();
			stmt.clearBatch();

			// commit the transaction (if we came this far)
			con.commit();
		} catch (SQLException e) {
			System.err.println("Error at line " + i + ": " + e.getMessage());
		}
	}

	/**
	 * Wrapper method that decides to dump SQL or XML.  In the latter case,
	 * this method does the XML data generation.
	 *
	 * @param out a Writer to write the data to
	 * @param table the table to dump
	 * @param dbmd the DatabaseMetaData to use
	 * @param stmt the Statement to use
	 * @throws SQLException if a database related error occurs
	 */
	public static void doDump(
		PrintWriter out,
		Table table,
		DatabaseMetaData dbmd,
		Statement stmt)
	throws SQLException	{
		e.dumpSchema(
			dbmd,
			table.getType(),
			table.getCat(),
			table.getSchem(),
			table.getName()
		);
		out.println();

		e.dumpResultSet(
			stmt.executeQuery("SELECT * FROM " + table.getFqnameQ())
		);
		out.println();
	}

	/**
	 * Simple helper method that generates a prompt.
	 *
	 * @param user the username
	 * @param stack the current SQLStack
	 * @param compl whether the statement is complete
	 * @return a prompt which consist of a username plus the top of the stack
	 */
	private static String getPrompt(String user, SQLStack stack, boolean compl) {
		return(user + (compl ? "-" : "=") +
			(stack.empty() ? ">" : "" + stack.peek()) + " ");
	}

	/**
	 * Scans the given string and tries to discover if it is a complete query
	 * or that there needs something to be added.  If a string doesn't end with
	 * a ; it is considered not to be complete.  SQL string quotation using ' and
	 * SQL identifier quotation using " is taken into account when scanning a
	 * string this way.
	 * Additionally, this method removes comments from the SQL statements,
	 * identified by -- and removes white space where appropriate.
	 *
	 * @param query the query to parse
	 * @return a QueryPart object containing the results of this parse
	 */
	private static QueryPart scanQuery(String query, SQLStack stack) {
		// examine string, char for char
		boolean wasInString = (stack.peek() == '\'');
		boolean wasInIdentifier = (stack.peek() == '"');
		boolean escaped = false;
		int len = query.length();
		for (int i = 0; i < len; i++) {
			switch(query.charAt(i)) {
				case '\\':
					escaped = !escaped;
				break;
				default:
					escaped = false;
				break;
				case '\'':
					/**
					 * We can not be in a string if we are in an identifier. So
					 * If we find a ' and are not in an identifier, and not in
					 * a string we can safely assume we will be now in a string.
					 * If we are in a string already, we should stop being in a
					 * string if we find a quote which is not prefixed by a \,
					 * for that would be an escaped quote. However, a nasty
					 * situation can occur where the string is like 'test \\'.
					 * As obvious, a test for a \ in front of a ' doesn't hold
					 * here. Because 'test \\\'' can exist as well, we need to
					 * know if a quote is prefixed by an escaping slash or not.
					 */
					if (!escaped && stack.peek() != '"') {
						if (stack.peek() != '\'') {
							// although it makes no sense to escape a quote
							// outside a string, it is escaped, thus not meant
							// as quote for us, apparently
							stack.push('\'');
						} else {
							stack.pop();
						}
					}
					// reset escaped flag
					escaped = false;
				break;
				case '"':
					if (!escaped && stack.peek() != '\'') {
						if (stack.peek() != '"') {
							stack.push('"');
						} else {
							stack.pop();
						}
					}
					// reset escaped flag
					escaped = false;
				break;
				case '-':
					if (!escaped && stack.peek() != '\'' && stack.peek() != '"' && i + 1 < len && query.charAt(i + 1) == '-') {
						len = i;
					}
					escaped = false;
				break;
				case '(':
					if (!escaped && stack.peek() != '\'' && stack.peek() != '"') {
						stack.push('(');
					}
					escaped = false;
				break;
				case ')':
					if (!escaped && stack.peek() == '(') {
						stack.pop();
					}
					escaped = false;
				break;
			}
		}

		int start = 0;
		if (!wasInString && !wasInIdentifier && len > 0) {
			// trim spaces at the start of the string
			for (; start < len && Character.isWhitespace(query.charAt(start)); start++);
		}
		int stop = len - 1;
		if (stack.peek() !=  '\'' && !wasInIdentifier && stop > start) {
			// trim spaces at the end of the string
			for (; stop >= start && Character.isWhitespace(query.charAt(stop)); stop--);
		}
		stop++;

		if (start == stop) {
			// we have an empty string
			return(new QueryPart(false, null, stack.peek() ==  '\'' || stack.peek() == '"'));
		} else if (stack.peek() ==  '\'' || stack.peek() == '"') {
			// we have an open quote
			return(new QueryPart(false, query.substring(start, stop), true));
		} else {
			// see if the string is complete
			if (query.charAt(stop - 1) == ';') {
				return(new QueryPart(true, query.substring(start, stop), false));
			} else {
				return(new QueryPart(false, query.substring(start, stop), false));
			}
		}
	}

	public static String dq(String in) {
		return("\"" + in.replaceAll("\\\\", "\\\\\\\\").replaceAll("\"", "\\\\\"") + "\"");
	}
}

/**
 * A QueryPart is (a part of) a SQL query.  In the QueryPart object information
 * like the actual SQL query string, whether it has an open quote and the like
 * is stored.
 */
class QueryPart {
	private boolean complete;
	private String query;
	private boolean open;

	public QueryPart(boolean complete, String query, boolean open) {
		this.complete = complete;
		this.query = query;
		this.open = open;
	}

	public boolean isEmpty() {
		return(query == null);
	}

	public boolean isComplete() {
		return(complete);
	}

	public String getQuery() {
		return(query);
	}

	public boolean hasOpenQuote() {
		return(open);
	}
}

/**
 * An SQLStack is a simple stack that keeps track of open brackets and
 * (single and double) quotes in an SQL query.
 */
class SQLStack {
	StringBuffer stack;

	public SQLStack() {
		stack = new StringBuffer();
	}

	public char peek() {
		if (empty()) {
			return('\0');
		} else {
			return(stack.charAt(stack.length() - 1));
		}
	}

	public char pop() {
		char tmp = peek();
		if (tmp != '\0') {
			stack.setLength(stack.length() - 1);
		}
		return(tmp);
	}

	public char push(char item) {
		stack.append(item);
		return(item);
	}

	public boolean empty() {
		return(stack.length() == 0);
	}
}

/**
 * A Table represents an SQL table.  All data required to
 * generate a fully qualified name is stored, as well as dependency
 * data.
 */
class Table {
	final String cat;
	final String schem;
	final String name;
	final String type;
	final String fqname;
	List needs;

	Table(String cat, String schem, String name, String type) {
		this.cat = cat;
		this.schem = schem;
		this.name = name;
		this.type = type;
		this.fqname = schem + "." + name;

		needs = new ArrayList();
	}

	void addDependancy(Table dependsOn) throws Exception {
		if (this.fqname.equals(dependsOn.fqname))
			throw new Exception("Cyclic dependancy graphs are not supported (foreign key relation references self)");

		if (dependsOn.needs.contains(this))
			throw new Exception("Cyclic dependancy graphs are not supported (foreign key relation a->b and b->a)");

		if (!needs.contains(dependsOn)) needs.add(dependsOn);
	}

	List requires(List existingTables) {
		if (existingTables == null || existingTables.size() == 0)
			return(new ArrayList(needs));

		List req = new ArrayList();
		for (int i = 0; i < needs.size(); i++) {
			if (!existingTables.contains(needs.get(i)))
				req.add(needs.get(i));
		}

		return(req);
	}

	String getCat() {
		return(cat);
	}

	String getSchem() {
		return(schem);
	}

	String getSchemQ() {
		return(JdbcClient.dq(schem));
	}

	String getName() {
		return(name);
	}

	String getNameQ() {
		return(JdbcClient.dq(name));
	}

	String getType() {
		return(type);
	}

	String getFqname() {
		return(fqname);
	}

	String getFqnameQ() {
		return(getSchemQ() + "." + getNameQ());
	}

	public String toString() {
		return(fqname);
	}


	static Table findTable(String fqname, List list) {
		for (int i = 0; i < list.size(); i++) {
			if (((Table)(list.get(i))).fqname.equals(fqname))
				return((Table)(list.get(i)));
		}
		// not found
		return(null);
	}

	static void checkForLoop(Table table, List parents) throws Exception {
		parents.add(table);
		for (int i = 0; i < table.needs.size(); i++) {
			Table child = (Table)(table.needs.get(i));
			if (parents.contains(child))
				throw new Exception("Cyclic dependancy graphs are not supported (cycle detected for " + child.fqname + ")");
			checkForLoop(child, parents);
		}
	}
}
