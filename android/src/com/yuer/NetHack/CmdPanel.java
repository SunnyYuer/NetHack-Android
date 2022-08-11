package com.yuer.NetHack;

import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import com.yuer.NetHack.R;
import com.yuer.NetHack.Cmd.KeySequnece;

public class CmdPanel
{
	private Activity mContext;
	private NH_State mState;
	private LinearLayout mBtnPanel;
	private Button mContextView;
	private int mMinBtnSize;
	private LayoutParams mParams;
	private NH_Dialog mEditDlg;
	private EditText mInput;
	private int mItemId;
	private CmdPanelLayout mLayout;
	private int mOpacity;
	private int mRelSize;

	// ____________________________________________________________________________________
	public CmdPanel(Activity context, NH_State state, CmdPanelLayout layout, String cmds, int opacity, int relSize)
	{
		mContext = context;
		mState = state;
		mLayout = layout;
		mBtnPanel = new LinearLayout(context);
		mBtnPanel.setLayoutParams(new ViewGroup.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		mOpacity = opacity;
		updateMinButtonSize(relSize);
		loadCmds(cmds);
	}

	private void updateMinButtonSize(int relSize) {
		final float density = mContext.getResources().getDisplayMetrics().density;
		mRelSize = relSize;
		int scale = mRelSize > 0 ? 2 : 1;
		mMinBtnSize = (int)((50 + scale * mRelSize) * density + 0.5f);
	}

	// ____________________________________________________________________________________
	private void loadCmds(String cmds)
	{
		mParams = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, mMinBtnSize);

		mBtnPanel.removeAllViews();

		Pattern p = Pattern.compile("\\s*((\\\\\\s|[^\\s])+)");
		Pattern p1 = Pattern.compile("((\\\\\\||[^\\|])+)");
		Matcher m = p.matcher(cmds);
		ArrayList<String> cmdList = new ArrayList<String>();
		while(m.find())
			cmdList.add(m.group(1));
		for(String c : cmdList)
		{
			m = p1.matcher(c);
			if(!m.find())
				continue;
			String cmd = m.group(1);
			String label = "";
			if(m.find())
				label = m.group(1);
			cmd = cmd.replace("\\ ", " ");
			label = label.replace("\\ ", " ");
			View v = createCmdButtonFromString(cmd, label);
			mBtnPanel.addView(v);
		}
	}

	// ____________________________________________________________________________________
	public String getCmds()
	{
		int nButtons = mBtnPanel.getChildCount();

		StringBuilder builder = new StringBuilder();
		for(int i = 0; i < nButtons; i++)
		{
			Button btn = (Button)mBtnPanel.getChildAt(i);
			Cmd cmd = (Cmd)btn.getTag();
			builder.append(cmd.getCommand().replace("|", "\\|").replace(" ", "\\ "));
			if(cmd.hasLabel())
				builder.append("|").append(cmd.getLabel().replace("|", "\\|").replace(" ", "\\ "));
			if(i != nButtons - 1)
				builder.append(' ');
		}
		return builder.toString();
	}

	// ____________________________________________________________________________________
	private Button createCmdButtonFromString(String chars, String label)
	{
		// special case for keyboard. not very intuitive perhaps
		if(chars.equalsIgnoreCase("..."))
		{
			return createCmdButtonFromCmd(new Cmd.ToggleKeyboard(mState, label));
		}
		// special case for menu.
		if(chars.equalsIgnoreCase("menu"))
		{
			return createCmdButtonFromCmd(new Cmd.OpenMenu(mState, label));
		}

		KeySequnece cmd = new Cmd.KeySequnece(mState, chars, label);
		return createCmdButtonFromCmd(cmd);
	}

	// ____________________________________________________________________________________
	private Button createCmdButton()
	{
		Button btn = new Button(mContext);
		btn.setTypeface(Typeface.MONOSPACE);
		btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
		btn.setMinimumWidth(mMinBtnSize);
		btn.setMinimumHeight(0);
		btn.setLayoutParams(mParams);
		btn.setFocusable(false);
		btn.setFocusableInTouchMode(false);

		mContext.registerForContextMenu(btn);

		return btn;
	}

	// ____________________________________________________________________________________
	private void updatePanelSize()
	{
		mParams.height = mMinBtnSize;

		int nButtons = mBtnPanel.getChildCount();
		for(int i = 0; i < nButtons; i++)
		{
			Button btn = (Button)mBtnPanel.getChildAt(i);
			btn.setMinimumWidth(mMinBtnSize);
			btn.requestLayout();
		}
	}

	// ____________________________________________________________________________________
	private void updatePanelOpacity()
	{
		int nButtons = mBtnPanel.getChildCount();
		for(int i = 0; i < nButtons; i++)
		{
			Button btn = (Button)mBtnPanel.getChildAt(i);
			btn.getBackground().setAlpha(mOpacity);
			if(mOpacity <= 127)
				btn.setTextColor(0xffffffff);
			else
				btn.setTextColor(0xff000000);
		}
	}

	// ____________________________________________________________________________________
	private Button createCmdButtonFromCmd(final Cmd cmd)
	{
		Button btn = createCmdButton();
		btn.setText(cmd.toString());
		btn.setTag(cmd);
		if(cmd.hasLabel())
			btn.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
		btn.getBackground().setAlpha(mOpacity);
		if(mOpacity <= 127)
			btn.setTextColor(0xffffffff);

		btn.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				mLayout.executeCmd(cmd);
			}
		});
		return btn;
	}

	// ____________________________________________________________________________________
	public void onCreateContextMenu(ContextMenu menu, View v)
	{
		if(v.getParent() != mBtnPanel)
			return;
		MenuInflater inflater = mContext.getMenuInflater();
		inflater.inflate(R.menu.customize_cmd, menu);
		Cmd cmd = (Cmd)v.getTag();
		String title = mContext.getString(R.string.command) + ": " + cmd.getCommand();
		if(cmd.hasLabel())
			title = title + " (" + cmd.getLabel() + ")";
		menu.setHeaderTitle(title);
		mContextView = (Button)v;
	}
	// ____________________________________________________________________________________
	public void onContextMenuClosed()
	{
		mContextView = null;
	}
	// ____________________________________________________________________________________
	public void onContextItemSelected(MenuItem item)
	{
		if(mContextView == null)
			return;

		mItemId = item.getItemId();

		if(mItemId == R.id.remove)
		{
			mBtnPanel.removeView(mContextView);
			mLayout.savePanelCmds(this);
		}
		else if(mItemId == R.id.add_kbd)
		{
			int idx = mBtnPanel.indexOfChild(mContextView);
			mBtnPanel.addView(createCmdButtonFromString("...", ""), idx);
			mLayout.savePanelCmds(this);
		}
		else if(mItemId == R.id.add_settings)
		{
			int idx = mBtnPanel.indexOfChild(mContextView);
			mBtnPanel.addView(createCmdButtonFromString("menu", mContext.getString(R.string.menu)), idx);
			mLayout.savePanelCmds(this);
		}
		else if(mItemId == R.id.label)
		{
			mInput = new EditText(mContext);
			mInput.setMaxLines(1);
			mInput.setSingleLine();
			mInput.setText(((Cmd)mContextView.getTag()).getLabel());
			mInput.selectAll();

			mEditDlg = new NH_Dialog(mContext, mContextView);
			mEditDlg.setTitle(mContext.getString(R.string.TypeCustomLabel));
			mEditDlg.setView(mInput);
			mEditDlg.setNegativeButton(mContext.getString(R.string.cancel), null);
			mEditDlg.setPositiveButton(mContext.getString(R.string.ok), onPositiveButton);
			mInput.setOnEditorActionListener(onEditorActionListener);
			mEditDlg.show();
		}
		else if(mItemId == R.id.opacity)
		{
			mLayout.openCmdPanelOpacity(this);
		}
		else if(mItemId == R.id.resize)
		{
			mLayout.openCmdPanelSize(this);
		}
		else
		{
			mInput = new EditText(mContext);
			mInput.setMaxLines(1);
			mInput.setSingleLine();
			if(mItemId == R.id.change)
				mInput.setText(((Cmd)mContextView.getTag()).getCommand());
			mInput.selectAll();

			mEditDlg = new NH_Dialog(mContext, mContextView);
			mEditDlg.setTitle(mContext.getString(R.string.TypeCommandSequence));
			mEditDlg.setView(mInput);
			mEditDlg.setNegativeButton(mContext.getString(R.string.cancel), null);
			mEditDlg.setPositiveButton(mContext.getString(R.string.ok), onPositiveButton);
			mInput.setOnEditorActionListener(onEditorActionListener);
			mEditDlg.show();
		}
		mContextView = null;
	}

	// ____________________________________________________________________________________
	private OnEditorActionListener onEditorActionListener = new OnEditorActionListener()
	{
		public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
		{
			if(event != null && event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode() == KeyEvent.KEYCODE_ENTER)
			{
				if(mEditDlg != null && mEditDlg.isShowing())
				{
					mEditDlg.getButton(NH_Dialog.BUTTON_POSITIVE).performClick();
					return true;
				}
			}
			return false;
		}
	};

	// ____________________________________________________________________________________
	private DialogInterface.OnClickListener onPositiveButton = new DialogInterface.OnClickListener()
	{
		@Override
		public void onClick(DialogInterface dialog, int whichButton)
		{
			View panelButton = ((NH_Dialog)dialog).getTag();
			int idx = mBtnPanel.indexOfChild(panelButton);
			if(idx >= 0)
			{
				String cmd = "";
				String label = "";
				if(mItemId == R.id.add)
				{
					cmd = mInput.getText().toString();
				}
				else if(mItemId == R.id.change)
				{
					cmd = mInput.getText().toString();
					label = ((Cmd)panelButton.getTag()).getLabel();
				}
				else if(mItemId == R.id.label)
				{
					cmd = ((Cmd)panelButton.getTag()).getCommand();
					label = mInput.getText().toString();
				}
				if(mItemId == R.id.change || mItemId == R.id.label)
					mBtnPanel.removeViewAt(idx);
				mBtnPanel.addView(createCmdButtonFromString(sanitize(cmd), sanitize(label)), idx);
				mBtnPanel.refreshDrawableState();
				mLayout.savePanelCmds(CmdPanel.this);
			}
		}

		private String sanitize(String cmd) {
			return cmd.replace("\n", "\\n").replace("\r", "\\n");
		}
	};

	// ____________________________________________________________________________________
	public void attach(ViewGroup newParent, boolean bHorizontal)
	{
		ViewGroup parent = (ViewGroup)mBtnPanel.getParent();
		if(parent != newParent)
		{
			if(parent != null)
				parent.removeView(mBtnPanel);
			newParent.addView(mBtnPanel);
			if(bHorizontal)
				mBtnPanel.setOrientation(LinearLayout.HORIZONTAL);
			else
				mBtnPanel.setOrientation(LinearLayout.VERTICAL);
		}

	}

	// ____________________________________________________________________________________
	public int getRelSize()
	{
		return mRelSize;
	}

	// ____________________________________________________________________________________
	public int getOpacity()
	{
		return mOpacity;
	}

	// ____________________________________________________________________________________
	public void setRelSize(int relSize)
	{
		if(mRelSize != relSize) {
			updateMinButtonSize(relSize);
			updatePanelSize();
		}
	}

	// ____________________________________________________________________________________
	public void setOpacity(int opacity)
	{
		if(mOpacity != opacity) {
			mOpacity = opacity;
			updatePanelOpacity();
		}
	}
}
