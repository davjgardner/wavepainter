import javax.swing.*;
import javax.swing.event.MouseInputAdapter;
import java.awt.*;
import java.awt.event.MouseEvent;

public class WavePainter {

	private static final int WIDTH = 320;
	private static final int HEIGHT = 240;
	private static final int NSAMPLES = 256;

	private JFrame frame;
	private JPanel drawPanel;
	private JPanel buttonPanel;

	private int[] waveData = new int[WIDTH];
	private byte[] data = new byte[NSAMPLES];
	
	private Point lastPos = new Point(0, 0);

	private WavePainter() {
		for (int i = 0; i < WIDTH; i++) {
			waveData[i] = HEIGHT / 2;
		}
		
		frame = new JFrame("WavePainter");
		frame.setSize(WIDTH, HEIGHT);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setLocationRelativeTo(null);

		drawPanel = new JPanel() {
			@Override
			public void paint(Graphics g) {
				super.paint(g);
				/* Apparently, `Graphics` defines its own `WIDTH` and `HEIGHT` variables
				     so we have to preface ours with `WavePainter.` */
				// draw a zero line
				g.setColor(Color.blue);
				g.drawLine(0, WavePainter.HEIGHT / 2, WavePainter.WIDTH, WavePainter.HEIGHT / 2);
				// draw the waveform
				g.setColor(Color.black);
				for (int i = 0; i < WavePainter.WIDTH - 1; i++) {
					g.drawLine(i, waveData[i], i + 1, waveData[i + 1]);
				}
			}
		};

		MouseInputAdapter mia = new MouseInputAdapter() {
			
			@Override
			public void mousePressed(MouseEvent e) {
				lastPos.x = e.getX();
				lastPos.y = e.getY();
			}
			
			@Override
			public void mouseReleased(MouseEvent e) {
				lastPos.x = -1;
				lastPos.y = -1;
			}
			
			@Override
			public void mouseClicked(MouseEvent e) {
				int x = e.getX();
				int y = e.getY();
				if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
					waveData[x] = y;
				}
			}

			@Override
			public void mouseDragged(MouseEvent e) {
				int x = e.getX();
				int y = e.getY();
				System.err.println("dragged to " + x + ", " + y);
				if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
					// lerp between the last point we got an event for and this point
					if (lastPos.x < x) {
						for (int i = lastPos.x; i <= x; i++) {
							waveData[i] = lastPos.y + (y - lastPos.y) * (i - lastPos.x) / (x - lastPos.x);
						}
					} else if (x < lastPos.x) {
						for (int i = x; i <= lastPos.x; i++) {
							waveData[i] = y + (lastPos.y - y) * (i - x) / (lastPos.x - x);
						}
					} else {
						// avoid a divide-by-zero
						waveData[x] = y;
					}
					lastPos.x = x;
					lastPos.y = y;
				}
				drawPanel.repaint();
			}
		};

		drawPanel.addMouseListener(mia);
		drawPanel.addMouseMotionListener(mia);

		buttonPanel = new JPanel();
		JButton setButton = new JButton("Set Wave");
		setButton.addActionListener(e -> sendData());

		buttonPanel.add(setButton);
		frame.setLayout(new BorderLayout());
		frame.add(drawPanel, BorderLayout.CENTER);
		frame.add(buttonPanel, BorderLayout.SOUTH);

		frame.setVisible(true);
	}

	private void sendData() {
		int max = 0;
		for (int i = 0; i < WIDTH; i++) {
			if (waveData[i] > max) max = waveData[i];
		}
		max -= WIDTH / 2;
		for (int i = 0; i < NSAMPLES; i++) {
			int x = i * WIDTH / NSAMPLES;
			byte val = (byte) ((waveData[x] - WIDTH / 2) * 127 / max);
			data[i] = val;
		}
		System.out.write('s');
		System.out.flush();
		System.out.write(data, 0, NSAMPLES);
	}

	public static void main(String[] args) {
		new WavePainter();
	}
}
