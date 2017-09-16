package net.skaldchen.commons.utils;

import java.util.Enumeration;

import javax.swing.JTable;
import javax.swing.table.JTableHeader;
import javax.swing.table.TableColumn;

public class JTableUtil {

    //�Զ������п�
    public static void fitTableColumns(JTable table) {
        JTableHeader header = table.getTableHeader();
        int rowCount = table.getRowCount();
        Enumeration<?> columns = table.getColumnModel().getColumns();
        while (columns.hasMoreElements()) {
            TableColumn column = (TableColumn) columns.nextElement();
            int col = header.getColumnModel().getColumnIndex(column.getIdentifier());
            int width = (int) table.getTableHeader().getDefaultRenderer()
                    .getTableCellRendererComponent(table,column.getIdentifier(), false, false, -1, col).getPreferredSize().getWidth();
            for (int row = 0; row < rowCount; row++) {
                int preferedWidth = (int) table.getCellRenderer(row, col)
                        .getTableCellRendererComponent(table,
                                table.getValueAt(row, col), false, false, row,col).getPreferredSize().getWidth();
                width = Math.max(width, preferedWidth);
            }
            header.setResizingColumn(column); // ���к���Ҫ
            column.setWidth(width + table.getIntercellSpacing().width);
        }
    }

    //�ֶ������п�
    public static void fitTableColumns(JTable table, int[] columnWidths) {
        for (int i = 0; i < columnWidths.length; i++) {
            table.getColumnModel().getColumn(i).setPreferredWidth(
                    columnWidths[i]);
        }
    }
}
