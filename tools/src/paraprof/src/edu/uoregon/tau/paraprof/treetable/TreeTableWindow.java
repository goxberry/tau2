package edu.uoregon.tau.paraprof.treetable;

import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.print.PageFormat;
import java.awt.print.Printable;
import java.awt.print.PrinterException;
import java.util.*;
import java.util.List;

import javax.swing.*;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TreeExpansionEvent;
import javax.swing.event.TreeExpansionListener;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableColumn;

import edu.uoregon.tau.paraprof.*;
import edu.uoregon.tau.paraprof.interfaces.ImageExport;
import edu.uoregon.tau.paraprof.interfaces.ParaProfWindow;
import edu.uoregon.tau.paraprof.interfaces.UnitListener;
import edu.uoregon.tau.paraprof.treetable.ColumnChooser.CheckBoxListItem;
import edu.uoregon.tau.paraprof.treetable.TreeTableColumn.*;

/**
 * Displays callpath data in a Cube style tree/table.  Non-callpath data is also displayed as a flat tree.
 * This is a replacement for the statistics text window.  It shows all that data and more, plus allows easy
 * sorting by column, reordering of columns, and choosing of columns.
 *    
 * TODO : ...
 *
 * <P>CVS $Id: TreeTableWindow.java,v 1.11 2005/09/19 21:49:43 amorris Exp $</P>
 * @author  Alan Morris
 * @version $Revision: 1.11 $
 */
public class TreeTableWindow extends JFrame implements TreeExpansionListener, Observer, ParaProfWindow, Printable, UnitListener,
        ImageExport {

    private CallPathModel model;
    private JTreeTable treeTable;
    private ParaProfTrial ppTrial;
    private JScrollPane scrollPane;
    private edu.uoregon.tau.dms.dss.Thread thread;
    private int colorMetricID;
    private int units = ParaProf.preferences.getUnits();

    private final JMenuItem showAsTreeMenuItem = new JCheckBoxMenuItem("Show as Call Path Tree");
    private final JMenuItem showInclExclMenuItem = new JCheckBoxMenuItem("Show Inclusive/Exclusive", true);

    private List columns;
    private ColumnChooser columnChooser;

    public TreeTableWindow(ParaProfTrial ppTrial, edu.uoregon.tau.dms.dss.Thread thread, Component invoker) {

        this.ppTrial = ppTrial;
        this.thread = thread;
        ppTrial.addObserver(this);

        // create the column chooser.  Note: the column chooser holds the data on which columns are shown
        columnChooser = new ColumnChooser(this, ppTrial);

        setSize(ParaProfUtils.checkSize(new Dimension(1000, 600)));

        setLocation(WindowPlacer.getNewLocation(this, invoker));

        if (thread.getNodeID() == -1) {
            this.setTitle("Mean Statistics - " + ppTrial.getTrialIdentifier(ParaProf.preferences.getShowPathTitleInReverse()));
        } else if (thread.getNodeID() == -2) {
            this.setTitle("Total Statistics - " + ppTrial.getTrialIdentifier(ParaProf.preferences.getShowPathTitleInReverse()));
        } else if (thread.getNodeID() == -3) {
            this.setTitle("Std. Dev. Statistics - " + ppTrial.getTrialIdentifier(ParaProf.preferences.getShowPathTitleInReverse()));
        } else {
            this.setTitle("Thread Statistics: " + "n,c,t, " + thread.getNodeID() + "," + thread.getContextID() + ","
                    + thread.getThreadID() + " - " + ppTrial.getTrialIdentifier(ParaProf.preferences.getShowPathTitleInReverse()));
        }

        if (ParaProf.helpWindow.isVisible()) {
            this.help(false);
        }

        setupMenus();
        setupData();

        ParaProf.incrementNumWindows();
    }

    public void updateColumns() {

        setColumns();

        AbstractTableModel atm = (AbstractTableModel) treeTable.getModel();
        atm.fireTableChanged(new TableModelEvent(atm, TableModelEvent.HEADER_ROW));

        // reset the cell rendereres
        for (int i = 0; i < columns.size(); i++) {
            treeTable.setDefaultRenderer(columns.get(i).getClass(), ((TreeTableColumn) columns.get(i)).getCellRenderer());
        }
    }

    // get the chosen columns from the columnChooser and setup the 'columns' List
    private void setColumns() {
        columns = new ArrayList();

        ListModel metricModel = columnChooser.getMetricModel();
        ListModel valueModel = columnChooser.getValueModel();

        for (int i = 0; i < metricModel.getSize() - 2; i++) { // -2 because the last two are calls and subr
            CheckBoxListItem item = (CheckBoxListItem) metricModel.getElementAt(i);
            if (item.getSelected()) {

                for (int j = 0; j < valueModel.getSize(); j++) {
                    CheckBoxListItem valueItem = (CheckBoxListItem) valueModel.getElementAt(j);
                    if (valueItem.getSelected()) {
                        String str = (String) valueItem.getUserObject();
                        if (str.equals("Absolute Value")) {
                            if (showInclExclMenuItem.isSelected()) {
                                columns.add(new InclusiveColumn(this, i));
                                columns.add(new ExclusiveColumn(this, i));
                            } else {
                                columns.add(new RegularMetricColumn(this, i));
                            }
                        } else if (str.equals("Percent Value")) {
                            if (showInclExclMenuItem.isSelected()) {
                                columns.add(new InclusivePercentColumn(this, i));
                                columns.add(new ExclusivePercentColumn(this, i));
                            } else {
                                columns.add(new RegularPercentMetricColumn(this, i));
                            }
                        } else if (str.equals("Absolute Value Per Call")) {
                            if (showInclExclMenuItem.isSelected()) {
                                columns.add(new InclusivePerCallColumn(this, i));
                                columns.add(new ExclusivePerCallColumn(this, i));
                            } else {
                                columns.add(new RegularPerCallMetricColumn(this, i));
                            }
                        }
                    }
                }
            }
        }

        CheckBoxListItem callitem = (CheckBoxListItem) metricModel.getElementAt(metricModel.getSize() - 2);
        CheckBoxListItem subritem = (CheckBoxListItem) metricModel.getElementAt(metricModel.getSize() - 1);

        if (callitem.getSelected()) {
            columns.add(new NumCallsColumn(this));
        }

        if (subritem.getSelected()) {
            columns.add(new NumSubrColumn(this));
        }
    }

    private void addComponents() {

        getContentPane().removeAll();
        getContentPane().setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();

        if (showAsTreeMenuItem.isSelected() == false) {
            showInclExclMenuItem.setEnabled(false);
            showInclExclMenuItem.setSelected(true);
        } else {
            showInclExclMenuItem.setEnabled(true);

            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.anchor = GridBagConstraints.NORTH;
            gbc.weightx = 1.0;
            gbc.weighty = 0.0;
            addCompItem(new ColorBar(), gbc, 0, 0, 1, 1);
        }

        if (scrollPane != null) {
            getContentPane().remove(scrollPane);
        }

        scrollPane = new JScrollPane(treeTable);

        for (int i = 0; i < columns.size(); i++) {
            treeTable.setDefaultRenderer(columns.get(i).getClass(), ((TreeTableColumn) columns.get(i)).getCellRenderer());
        }

        gbc.fill = GridBagConstraints.BOTH;
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        addCompItem(scrollPane, gbc, 0, 1, GridBagConstraints.REMAINDER, GridBagConstraints.REMAINDER);

        validate();
    }

    private void setupData() {

        setColumns();

        //columns.add(new StdDevColumn(this, 0));
        //columns.add(new MiniHistogramColumn(this));

        model = new CallPathModel(this, ppTrial, thread);
        createTreeTable(model);
        addComponents();

    }

    private void addCompItem(Component c, GridBagConstraints gbc, int x, int y, int w, int h) {
        gbc.gridx = x;
        gbc.gridy = y;
        gbc.gridwidth = w;
        gbc.gridheight = h;
        getContentPane().add(c, gbc);
    }

    private void setupMenus() {

        JMenuBar mainMenu = new JMenuBar();
        JMenu optionsMenu = new JMenu("Options");

        ActionListener actionListener = new ActionListener() {

            public void actionPerformed(ActionEvent evt) {
                try {
                    Object EventSrc = evt.getSource();

                    if (EventSrc instanceof JMenuItem) {

                        String arg = evt.getActionCommand();

                        if (arg.equals("Show as Call Path Tree")) {
                            if (showAsTreeMenuItem.isSelected()) {
                                showInclExclMenuItem.setSelected(false);
                            }

                            setupData();
                        } else if (arg.equals("Show Inclusive/Exclusive")) {
                            setupData();
                        }

                    }
                } catch (Exception e) {
                    ParaProfUtils.handleException(e);
                }

            }

        };

        showAsTreeMenuItem.addActionListener(actionListener);
        optionsMenu.add(showAsTreeMenuItem);
        showAsTreeMenuItem.setSelected(true);

        showInclExclMenuItem.addActionListener(actionListener);
        optionsMenu.add(showInclExclMenuItem);

        JMenu unitsSubMenu = ParaProfUtils.createUnitsMenu(this, units, false);
        optionsMenu.add(unitsSubMenu);

        JMenuItem menuItem = new JMenuItem("Choose Columns...");
        menuItem.addActionListener(new ActionListener() {

            public void actionPerformed(ActionEvent e) {
                columnChooser.showDialog(TreeTableWindow.this, true);
            }

        });

        optionsMenu.add(menuItem);

        if (!ppTrial.callPathDataPresent()) {
            showAsTreeMenuItem.setSelected(false);
            showAsTreeMenuItem.setEnabled(false);
            showInclExclMenuItem.setSelected(true);
            showInclExclMenuItem.setEnabled(false);
        }

        mainMenu.add(ParaProfUtils.createFileMenu((ParaProfWindow) this, this, this));
        mainMenu.add(optionsMenu);
        mainMenu.add(ParaProfUtils.createWindowsMenu(ppTrial, this));
        mainMenu.add(ParaProfUtils.createHelpMenu(this, this));

        setJMenuBar(mainMenu);

    }

    private void createTreeTable(AbstractTreeTableModel model) {
        //        treeTable = new JTreeTable(model, showAsTreeMenuItem.isSelected());
        treeTable = new JTreeTable(model, true, this);

        treeTable.getTree().addTreeExpansionListener(this);
        treeTable.getTree().setCellRenderer(new TreePortionCellRenderer());

        treeTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);
        treeTable.setAutoResizeMode(JTable.AUTO_RESIZE_SUBSEQUENT_COLUMNS);

        TableColumn col = treeTable.getColumnModel().getColumn(0);

        int nameWidth = 500;
        

        if (ppTrial.getNumberOfMetrics() > 1) {
            nameWidth = 350;
        }

        if (ParaProf.demoMode) {
            nameWidth = 200;
        }

        col.setPreferredWidth(nameWidth);

        int numCols = treeTable.getColumnCount();
        col = treeTable.getColumnModel().getColumn(numCols - 2);
        col.setPreferredWidth(10);
        col = treeTable.getColumnModel().getColumn(numCols - 1);
        col.setPreferredWidth(10);
    }

    public void treeCollapsed(TreeExpansionEvent event) {
        TreeTableNode node = (TreeTableNode) event.getPath().getLastPathComponent();
        node.setExpanded(false);
    }

    public void treeExpanded(TreeExpansionEvent event) {
        TreeTableNode node = (TreeTableNode) event.getPath().getLastPathComponent();
        node.setExpanded(true);
    }

    public void update(Observable o, Object arg) {
        setupData();
    }

    public void help(boolean display) {
        //Show the ParaProf help window.
        ParaProf.helpWindow.clearText();
        if (display)
            ParaProf.helpWindow.show();
        ParaProf.helpWindow.writeText("This is the Statistics Table.\n");
        ParaProf.helpWindow.writeText("This window shows you function data across a given thread (or mean/std.dev.)\n");
        ParaProf.helpWindow.writeText("If callpath data is present, it will be shown as a tree on the left.");
        ParaProf.helpWindow.writeText("In this mode, the metric values will show Inclusive when the node is collapsed, and exclusive when the node is expanded.\n");
    }

    public void closeThisWindow() {
        setVisible(false);
        ppTrial.deleteObserver(this);
        ParaProf.decrementNumWindows();
        dispose();
    }

    public int print(Graphics graphics, PageFormat pageFormat, int pageIndex) throws PrinterException {
        try {
            if (pageIndex >= 1) {
                return NO_SUCH_PAGE;
            }

            ParaProfUtils.scaleForPrint(graphics, pageFormat, (int) treeTable.getSize().getWidth(),
                    (int) treeTable.getSize().getHeight());
            export((Graphics2D) graphics, false, true, false);

            return Printable.PAGE_EXISTS;
        } catch (Exception e) {
            new ParaProfErrorDialog(e);
            return NO_SUCH_PAGE;
        }
    }

    public boolean getExclusiveInclusiveMode() {
        return showInclExclMenuItem.isSelected();
    }

    public boolean getTreeMode() {
        return showAsTreeMenuItem.isSelected();
    }

    public int getColorMetricID() {
        return colorMetricID;
    }

    public void setUnits(int units) {
        this.units = units;
        treeTable.forceRedraw();
    }

    public int getUnits() {
        return units;
    }

    public List getColumns() {
        return columns;
    }

    public ParaProfTrial getPPTrial() {
        return ppTrial;
    }

    public void export(Graphics2D g2D, boolean toScreen, boolean fullWindow, boolean drawHeader) {
        if (fullWindow) {
            treeTable.paintAll(g2D);
        } else {
            scrollPane.paintAll(g2D);
        }
    }

    public Dimension getImageSize(boolean fullScreen, boolean header) {
        if (fullScreen) {
            return scrollPane.getSize();
        } else {
            return treeTable.getSize();
        }
    }

    public edu.uoregon.tau.dms.dss.Thread getThread() {
        return thread;
    }

}
